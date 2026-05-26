#include "mmap.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

namespace fs = std::filesystem;


MMapAux::MMapAux(fpath file_path) : file_path_(file_path), mmap_data_head_(nullptr), capacity_(0) {
    page_size_ = getpagesize();
    file_fd_ = open(file_path_.string().c_str(), O_CREAT | O_RDWR, 0666);
    if (file_fd_ < 0) {
        return ;
    }
    size_t file_size = fs::file_size(file_path);
    Init();
}

MMapAux::~MMapAux() {
    close(file_fd_);
}

void MMapAux::Reserve(size_t new_size) {


}


void MMapAux::Init() {
    mmap_data_head_ = mmap(NULL, capacity_, PROT_WRITE, MAP_SHARED, file_fd_, 0);
    if (mmap_data_head_ == MAP_FAILED) {
        close(file_fd_);
        return ;
    }

    void* base = mmap_data_head_;
    MMapHeader* header = reinterpret_cast<MMapHeader*>(base);
}

size_t MMapAux::GetSetUpCapacity(size_t payload_size) {
    return ((payload_size + sizeof(MMapHeader) + page_size_ - 1) / page_size_) * page_size_;
}
