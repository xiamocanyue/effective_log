#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>

class MMapAux {
public:
    enum class AppendResult {
        kOk,
        kNoSpace,
        kInvalidArgument,
        kMapError
    };

    static constexpr size_t kDefaultPayloadLimit = 4 * 1024 * 1024;

    struct MMapHeader {
        static constexpr uint32_t kMagic = 0xdeadbeef;
        uint32_t magic;
        uint32_t size;

        MMapHeader() : magic(kMagic), size(0) {}
    };

    using fpath = std::filesystem::path;

    explicit MMapAux(fpath file_path);
    MMapAux(fpath file_path, size_t payload_limit);

    ~MMapAux();

    MMapAux(const MMapAux& other) = delete;

    MMapAux& operator=(const MMapAux& other) = delete;

    //返回写入内容的size， 即header->size()，不包括header的大小
    size_t Size() const;

    //这里的new_size是要存储输入的大小，不包括header,确保payload达到new_size
    ssize_t Push(const void* data, size_t data_size);

    AppendResult TryAppendRecord(const void* header, size_t header_size,
                                 const void* payload, size_t payload_size);

    void Clear();

    bool Empty() const;
    //返回数据区的起始地址
    uint8_t* Data();

    double GetRatio() const;

    size_t PayloadLimit() const;
    size_t MappedPayloadCapacity() const;
    size_t Remaining() const;
    bool CanAppend(size_t data_size) const;
    bool Valid() const;

private :
    void Init();
    bool Reserve(size_t payload_size);
    size_t GetSetUpCapacity(size_t payload_size);
    MMapHeader* GetHeader() const;
    bool Unmmap();
    bool MapFile(size_t new_capacity);


private:
    size_t capacity_;   //整体mmap的容量，包括header
    int page_size_;     //页面大小，用来对齐页
    void* mmap_data_head_;   //整体mmap的起始指针，也就是header的头指针
    fpath file_path_;   // file文件的地址
    int file_fd_;       //file 打开后的文件描述符。
    size_t payload_limit_;
    bool valid_;
};
