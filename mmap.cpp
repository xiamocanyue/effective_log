#include "mmap.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

namespace fs = std::filesystem;
//默认的容量，整个mmap，包括了header，和file_size对应
static const size_t kDefaultCapacity = 1024 * 512;

MMapAux::MMapAux(fpath file_path) : MMapAux(std::move(file_path), kDefaultPayloadLimit) {}

MMapAux::MMapAux(fpath file_path, size_t payload_limit)
    : capacity_(0), page_size_(getpagesize()), mmap_data_head_(nullptr),
      file_path_(std::move(file_path)), file_fd_(-1), payload_limit_(payload_limit), valid_(false) {
    if (payload_limit_ == 0 || payload_limit_ > std::numeric_limits<uint32_t>::max()) {
        return;
    }
    file_fd_ = open(file_path_.string().c_str(), O_CREAT | O_RDWR, 0666);
    if (file_fd_ < 0) {
        return ;
    }
    Init();
}

MMapAux::~MMapAux() {
    Unmmap();
    if (file_fd_ != -1) {
        close(file_fd_);
    }
}

ssize_t GetFileSize(int file_fd) {
    struct stat st;
    if (fstat(file_fd, &st) == -1) {
        perror("fstat 失败");
        return -1;
    }

    return st.st_size;
}


void MMapAux::Init() {
    ssize_t file_size = GetFileSize(file_fd_);   
    if (file_size < 0) {
        return ;
    }
    //文件大小  《  文件头的大小，代表肯定是不符合可续用文件的，初始化
    if (file_size < sizeof(MMapHeader)) {
        const size_t initial_payload = std::min(payload_limit_, kDefaultCapacity - sizeof(MMapHeader));
        capacity_ = GetSetUpCapacity(initial_payload);

        if (ftruncate(file_fd_, capacity_) < 0) {
            perror("ftruncate 文件扩容失败");
            return ;
        }

        mmap_data_head_ = mmap(NULL, capacity_, PROT_WRITE | PROT_READ, MAP_SHARED, file_fd_, 0);
        if (mmap_data_head_ == MAP_FAILED) {
            mmap_data_head_ = nullptr;
            capacity_ = 0;
            return ;
        }
        GetHeader()->magic = MMapHeader::kMagic;
        GetHeader()->size = 0;
        valid_ = true;
        return ;
    } 

    //文件大小 》= 文件头大小， 可能是可续用文件，先按照文件大小映射，查看是否可续用
    capacity_ = file_size;
    mmap_data_head_ = mmap(NULL, capacity_, PROT_WRITE | PROT_READ, MAP_SHARED, file_fd_, 0);
    if (mmap_data_head_ == MAP_FAILED) {
        mmap_data_head_ = nullptr;
        capacity_ = 0;
        return ;
    }

    MMapHeader* header = GetHeader();
    if (header == nullptr) {
        return ;
    }
        //虽然kmgic对上了，不过文件大小和实际的容量不匹配，初始化为一个空文件
    if (header->magic == MMapHeader::kMagic) {
        if (header->size + sizeof(MMapHeader) > capacity_ || header->size > payload_limit_) {
            Unmmap();
            return;
        }
    } else {
        header->magic = MMapHeader::kMagic;
        header->size = 0;
    }
    valid_ = true;

}

size_t MMapAux::GetSetUpCapacity(size_t payload_size) {
    return ((payload_size + sizeof(MMapHeader) + page_size_ - 1) / page_size_) * page_size_;
}


MMapAux::MMapHeader* MMapAux::GetHeader() const {
    if (mmap_data_head_ == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<MMapHeader*>(mmap_data_head_);
}

size_t MMapAux::Size() const {
    MMapHeader* header = GetHeader();
    if (header == nullptr) {
        return 0;
    }
    return header->size;
}

bool MMapAux::Empty() const {
    MMapHeader* header = GetHeader();
    if (header == nullptr) {
        return true;
    }
    return header->size == 0;
}

void MMapAux::Clear() {
    MMapHeader* header = GetHeader();
    if (header == nullptr) {
        return ;
    }
    header->size = 0;
}

uint8_t* MMapAux::Data() {
    if (mmap_data_head_ == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<uint8_t*>(mmap_data_head_) + sizeof(MMapHeader);
}


ssize_t MMapAux::Push(const void* data, size_t data_size) {
    if (data == nullptr && data_size > 0) {
        return -1;
    }

    MMapHeader* header = GetHeader();
    if (header == nullptr) {
        return -1;
    }
    if (data_size > Remaining()) {
        return -1;
    }
    size_t new_size = data_size + Size();
    if (!Reserve(new_size)) {
        return -1;
    }
    header = GetHeader();
    if (header == nullptr) {
        return -1;
    }
    uint8_t* begin = Data();
    if (begin == nullptr) {
        return -1;
    }
    uint8_t* cur = begin + header->size;
    std::memcpy(cur, data, data_size);
    header->size = new_size;
    return data_size;
}

MMapAux::AppendResult MMapAux::TryAppendRecord(const void* header, size_t header_size,
                                               const void* payload, size_t payload_size) {
    if ((header == nullptr && header_size > 0) || (payload == nullptr && payload_size > 0)) {
        return AppendResult::kInvalidArgument;
    }
    if (header_size > std::numeric_limits<size_t>::max() - payload_size) {
        return AppendResult::kNoSpace;
    }
    const size_t record_size = header_size + payload_size;
    if (!CanAppend(record_size)) {
        return AppendResult::kNoSpace;
    }
    const size_t old_size = Size();
    if (!Reserve(old_size + record_size)) {
        return AppendResult::kMapError;
    }
    MMapHeader* mmap_header = GetHeader();
    uint8_t* destination = Data() + old_size;
    if (header_size > 0) {
        std::memcpy(destination, header, header_size);
    }
    if (payload_size > 0) {
        std::memcpy(destination + header_size, payload, payload_size);
    }
    mmap_header->size = static_cast<uint32_t>(old_size + record_size);
    return AppendResult::kOk;
}



bool MMapAux::Reserve(size_t payload_size) {
    if (!valid_ || payload_size > payload_limit_) {
        return false;
    }
    if (payload_size + sizeof(MMapHeader) <= capacity_) {
        return true;
    }
    const size_t current_payload = MappedPayloadCapacity();
    size_t target_payload = current_payload;
    while (target_payload < payload_size) {
        if (target_payload > payload_limit_ / 2) {
            target_payload = payload_limit_;
        } else {
            target_payload *= 2;
        }
    }
    target_payload = std::max(target_payload, payload_size);
    size_t new_capacity = GetSetUpCapacity(target_payload);
    if (ftruncate(file_fd_, new_capacity) == -1) {
        perror("文件扩容失败");
        return false;
    }

    if (!Unmmap()) {
        return false;
    }
    if (!MapFile(new_capacity)) {
        return false;
    }

    return true;
}


bool MMapAux::Unmmap() {
    if (mmap_data_head_ != nullptr) {
        if (munmap(mmap_data_head_, capacity_) == -1) {
            return false;
        }        
        capacity_ = 0;
        mmap_data_head_ = nullptr;
    } 
    return true;
}

bool MMapAux::MapFile(size_t new_capacity) {
    mmap_data_head_ = mmap(NULL, new_capacity, PROT_READ | PROT_WRITE, MAP_SHARED, file_fd_, 0);
    if (mmap_data_head_ == MAP_FAILED) {
        mmap_data_head_ = nullptr;
        capacity_ = 0;
        return false;
    }
    capacity_ = new_capacity;
    return true;
}

double MMapAux::GetRatio() const {
    if (!valid_ || payload_limit_ == 0) {
        return 0.0;
    }
    return static_cast<double>(Size()) / static_cast<double>(payload_limit_);
}

size_t MMapAux::PayloadLimit() const {
    return payload_limit_;
}

size_t MMapAux::MappedPayloadCapacity() const {
    return capacity_ > sizeof(MMapHeader) ? std::min(payload_limit_, capacity_ - sizeof(MMapHeader)) : 0;
}

size_t MMapAux::Remaining() const {
    const size_t size = Size();
    return size <= payload_limit_ ? payload_limit_ - size : 0;
}

bool MMapAux::CanAppend(size_t data_size) const {
    return valid_ && data_size <= Remaining();
}

bool MMapAux::Valid() const {
    return valid_;
}
