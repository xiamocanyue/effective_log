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
#include <deque>
#include <fstream>

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
    struct PendingLogMsg {
        LogLevel level;
        std::string file_name;
        int32_t line;
        std::string func_name;
        std::string message;
        
        PendingLogMsg(){}
        PendingLogMsg(LogMsg& msg) : level(msg.level_), file_name(msg.source_location_.file_name_),
                                    line(msg.source_location_.line_), func_name(msg.source_location_.func_name_),
                                    message(msg.msg_) {}
    };

    explicit EffectiveSink(Conf conf);
    ~EffectiveSink();
    void Log(LogMsg& msg) override;
    void SetFormatter(std::unique_ptr<Formatter> formatter) override;
    void Flush() override;
    void Stop();

private:
    void WriteToCache(void* data, size_t data_size);
    bool NeedCacheToFile();
    bool TrySwapCacheForFlush();
    void CacheToFile();
    void PostSlaveTaskToWork();

    std::filesystem::path GetLogPath();
    void SwapCache();
    void EliminateTotalLogFileSize();

    void Start();
    void CreatAndAddThread();
    void ThreadWork(PendingLogMsg& msg);


private:
    std::unique_ptr<Formatter> formatter_;
    std::unique_ptr<Compress::Compression> compress_;
    std::unique_ptr<Crypt::BaseCrypt> crypt_;
    std::unique_ptr<MMapAux> master_cache_;
    std::unique_ptr<MMapAux> slave_cache_;
    std::string self_public_key_;
    std::string compress_output_buf_;
    std::string crypt_output_buf_;
    std::atomic<bool> is_slave_free_{true};
    std::mutex mutex_;
    Conf conf_;
    Executor::TaskRunnerTag task_runner_tag_;
    std::filesystem::path log_file_path_;
    std::ofstream ofs_;

    std::deque<PendingLogMsg> task_queue_;
    std::mutex queue_mutex_;
    std::vector<std::thread> work_threads_;
    std::condition_variable queue_cv_;
    std::atomic<bool> is_stop_{false};
    std::atomic<bool> is_accepting_{true};
    std::atomic<bool> is_running{false};
    std::atomic<int> active_workers_{0};
    std::atomic<int> thread_count_{10};
};



}
