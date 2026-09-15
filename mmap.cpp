#include "mmap.h"

#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

namespace fs = std::filesystem;
//默认的容量，整个mmap，包括了header，和file_size对应
static const size_t kDefaultCapacity = 1024 * 512;

MMapAux::MMapAux(fpath file_path) : file_path_(file_path), mmap_data_head_(nullptr), capacity_(0), file_fd_(-1) {
    page_size_ = getpagesize();
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
        capacity_ = GetSetUpCapacity((kDefaultCapacity - sizeof(MMapHeader)));

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
        if (header->size + sizeof(MMapHeader) > capacity_) {
            header->size = 0;
        }
    } else {
        header->magic = MMapHeader::kMagic;
        header->size = 0;
    }
 

}

size_t MMapAux::GetSetUpCapacity(size_t payload_size) {
    return ((payload_size + sizeof(MMapHeader) + page_size_ - 1) / page_size_) * page_size_;
}


MMapAux::MMapHeader* MMapAux::GetHeader() {
    if (mmap_data_head_ == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<MMapHeader*>(mmap_data_head_);
}

size_t MMapAux::Size() {
    MMapHeader* header = GetHeader();
    if (header == nullptr) {
        return 0;
    }
    return header->size;
}

bool MMapAux::Empty() {
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
    size_t new_size = data_size + Size();
    if (new_size + sizeof(MMapHeader) > capacity_) {
        if (!Reserve(new_size * 2)) {

            return -1;
        }
        header = GetHeader();
    }
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


bool MMapAux::Reserve(size_t new_size) {
    if (new_size + sizeof(MMapHeader) <= capacity_) {
        return true;
    }
    size_t new_capacity = GetSetUpCapacity(new_size);
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

double MMapAux::GetRatio() {
    if (mmap_data_head_ == nullptr) {
        return 0.0;
    }
    return static_cast<double>(Size()) / static_cast<double>((capacity_ - sizeof(MMapHeader)));
}