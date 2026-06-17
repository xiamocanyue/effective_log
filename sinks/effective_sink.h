#pragma once 
#include "sinks/sink.h"
#include "formatter/effective_formatter.h"
#include "crypt/crypt.h"
#include "compress/compress.h"
#include "mmap.h"
#include "space.h"
#include "context/context.h"

#include <mutex>
#include <atomic>

namespace Logger {
namespace Detail {

struct ItemHead {
    static constexpr uint32_t kMagic = 0xbe5fball;
    uint32_t magic;
    uint32_t size;

    ItemHead() : magic(kMagic), size(0) {}
}; 

struct ChunkHead {
    static constexpr uint64_t kMagic = 0xdeadbeefdada1100;
    uint64_t magic;
    uint64_t size;
    char pub_key[132];

    ChunkHead() : magic(kMagic), size(0) {}
};
}


class EffectiveSink : public LogSink {
public:
    struct Conf {
        std::string other_public_key;
        std::filesystem::path dir;
        std::string file_name;
        std::chrono::minutes passed_time_interval;
        megabytes single_size{4};
        megabytes total_size{100};
    };
    explicit EffectiveSink(Conf conf);
    void Log(LogMsg& msg) override;
    void SetFormatter(std::unique_ptr<Formatter> formatter) override;
    void Flush() override;

private:
    void WriteToCache(void* data, size_t data_size);
    bool NeedCacheToFile();
    void CacheToFile();
    void PostSlaveTaskToWork();

    std::filesystem::path GetLogPath();
    void SwapCache();
    void EliminateTotalLogFileSize();


private:
    std::unique_ptr<Formatter> formatter_;
    std::unique_ptr<Compress::Compression> compress_;
    std::unique_ptr<Crypt::BaseCrypt> crypt_;
    std::unique_ptr<MMapAux> master_cache_;
    std::unique_ptr<MMapAux> slave_cache_;
    std::string self_public_key_;
    std::string compress_output_buf_;
    std::string crypt_output_buf_;
    std::filesystem::path log_file_path_;
    std::atomic<bool> is_slave_free_{true};
    std::mutex mutex_;
    Conf conf_;
    Executor::TaskRunnerTag task_runner_tag_;
};



}
