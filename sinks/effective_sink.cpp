#include "sinks/effective_sink.h"
#include "crypt/aes_crypt.h"
#include "compress/zstd_compress.h"
#include "internal_log.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace Logger {

EffectiveSink::EffectiveSink(Conf conf) : conf_(std::move(conf)) {
    if (conf_.worker_threads <= 0) {
        throw std::invalid_argument("EffectiveSink worker_threads must be positive");
    }
    if (conf_.max_pending_bytes == 0 || conf_.critical_reserve_bytes == 0 ||
        conf_.max_record_bytes == 0 || conf_.cache_payload_limit == 0 ||
        conf_.cache_flush_ratio <= 0.0 || conf_.cache_flush_ratio >= 1.0) {
        throw std::invalid_argument("EffectiveSink capacity settings must be positive and cache_flush_ratio must be in (0, 1)");
    }
    if (conf_.critical_reserve_bytes >= conf_.max_pending_bytes) {
        throw std::invalid_argument("EffectiveSink critical_reserve_bytes must be less than max_pending_bytes");
    }
    if (conf_.max_record_bytes > conf_.max_pending_bytes) {
        throw std::invalid_argument("EffectiveSink max_record_bytes must not exceed max_pending_bytes");
    }
    if (conf_.cache_payload_limit > std::numeric_limits<uint32_t>::max()) {
        throw std::invalid_argument("EffectiveSink cache_payload_limit exceeds mmap format limit");
    }
    cache_flush_threshold_bytes_ = static_cast<size_t>(
        static_cast<long double>(conf_.cache_payload_limit) * conf_.cache_flush_ratio);
    if (cache_flush_threshold_bytes_ == 0) {
        cache_flush_threshold_bytes_ = 1;
    }
    LOG_INFO("~~~~ EffectiveSink: dir = {}, file_name = {}, other_public_key = {}, passed_time_interval = {}, single_size = {}, total_size = {}",
             conf_.dir.string(), conf_.file_name, conf_.other_public_key, conf_.passed_time_interval.count(), conf_.single_size.count(), conf_.total_size.count());
    if (!std::filesystem::exists(conf_.dir)) {
        std::filesystem::create_directories(conf_.dir);
    }
    task_runner_tag_ = NEW_TASK_RUNNER(10086);
    formatter_ = std::make_unique<EffectiveFormatter>();
    auto ecdh_key_pair = Crypt::GenECDHKey();
    auto self_private_key = std::get<0>(ecdh_key_pair);
    self_public_key_ = Crypt::BinaryToHex(std::get<1>(ecdh_key_pair));
    auto other_public_key = Crypt::HexToBinary(conf_.other_public_key);
    auto aes_shared_key = Crypt::GenECDHAgreeKey(self_private_key, other_public_key);
    crypt_ = std::make_unique<Crypt::AESCrypt>(aes_shared_key);
    compress_ = std::make_unique<Compress::ZstdCompression>();
    master_cache_ = std::make_unique<MMapAux>("master_cache", conf_.cache_payload_limit);
    slave_cache_ = std::make_unique<MMapAux>("slave_cache", conf_.cache_payload_limit);
    if (!master_cache_->Valid() || !slave_cache_->Valid()) {
        throw std::runtime_error("EffectiveSink mmap cache initialization failed");
    }
    Start();

    if (!slave_cache_->Empty()) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            is_slave_free_ = false;
        }
        PostSlaveTaskToWork();
        WAIT_TASK_IDLE(task_runner_tag_);
    }
    if (!master_cache_->Empty()) {
        bool need_post = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (is_slave_free_) {
                is_slave_free_ = false;
                SwapCache();
                need_post = true;
            }
        }
        if (need_post) {
            PostSlaveTaskToWork();
        }
    }
    POST_REPEATED_TASK(-1, conf_.passed_time_interval, task_runner_tag_, [this]() {EliminateTotalLogFileSize();});
}

EffectiveSink::~EffectiveSink() {
    Stop();
    ofs_.close();    
}

bool EffectiveSink::NeedCacheToFile() {
    return master_cache_->Size() >= cache_flush_threshold_bytes_;
}

void EffectiveSink::Log(LogMsg& msg) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (is_stop_.load() || !is_running.load() || !is_accepting_.load() || !IsAcceptedLevel(msg.level_)) {
        return ;
    }
    size_t accounted_bytes = sizeof(PendingLogMsg);
    const auto add_size = [&accounted_bytes](size_t value) {
        if (value > std::numeric_limits<size_t>::max() - accounted_bytes) {
            return false;
        }
        accounted_bytes += value;
        return true;
    };
    if (!add_size(msg.source_location_.file_name_.size()) ||
        !add_size(msg.source_location_.func_name_.size()) ||
        !add_size(msg.msg_.size()) || accounted_bytes > conf_.max_record_bytes) {
        return;
    }
    const bool critical = IsCritical(msg.level_);
    const size_t capacity = critical ? conf_.max_pending_bytes
                                     : conf_.max_pending_bytes - conf_.critical_reserve_bytes;
    if (pending_bytes_ > capacity || accounted_bytes > capacity - pending_bytes_) {
        return;
    }
    try {
        task_queue_.emplace_back(msg);
        task_queue_.back().accounted_bytes = accounted_bytes;
    } catch (...) {
        return;
    }
    pending_bytes_ += accounted_bytes;
    if (!encoder_busy_ && !cache_backpressure_ && !worker_wakeup_pending_) {
        worker_wakeup_pending_ = true;
        queue_cv_.notify_one();
    }
}

void EffectiveSink::Start() {
    if (is_running.load()) {
        return ;
    }
    if (is_stop_.load()) {
        return ;
    }
    is_running.store(true);
    for (int i = 0; i < conf_.worker_threads; i++) {
        CreatAndAddThread();
    }

}

void EffectiveSink::Stop() {
    if (!is_running.load()) {
        return ;
    }
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        is_accepting_.store(false);
    }
    if (is_stop_.load()) {
        return ;
    }
    this->Flush();
    {
        // The stop predicate must change under the same mutex used by wait(),
        // otherwise a worker can miss the notification and never exit.
        std::lock_guard<std::mutex> lock(queue_mutex_);
        is_stop_.store(true);
    }
    queue_cv_.notify_all();
    for (auto& thread : work_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    work_threads_.clear();
    is_running.store(false);
}

void EffectiveSink::CreatAndAddThread() {
    auto func = [this]() {
        while (true) {
            PendingLogMsg task_msg;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                queue_cv_.wait(lock, [this](){
                    return is_stop_.load() ||
                           (!task_queue_.empty() && !cache_backpressure_ && !encoder_busy_);
                });
                if (task_queue_.empty() && is_stop_.load()) {
                    break;
                }
                if (task_queue_.empty() || cache_backpressure_ || encoder_busy_) {
                    continue;
                }
                task_msg = std::move(task_queue_.front());
                task_queue_.pop_front();
                in_flight_bytes_ += task_msg.accounted_bytes;
                worker_wakeup_pending_ = false;
                encoder_busy_ = true;
                ++active_workers_;
            }
            try {
                ThreadWork(task_msg);
            } catch (const std::exception& exception) {
                LOG_ERROR("EffectiveSink worker exception: {}", exception.what());
            } catch (...) {
                LOG_ERROR("EffectiveSink worker unknown exception");
            }
            {
                // Flush waits for this predicate using queue_mutex_. An atomic
                // alone does not close the predicate-check / wait race.
                std::lock_guard<std::mutex> lock(queue_mutex_);
                in_flight_bytes_ -= task_msg.accounted_bytes;
                pending_bytes_ -= task_msg.accounted_bytes;
                encoder_busy_ = false;
                --active_workers_;
            }
            flush_cv_.notify_all();
        }

    };
    
    work_threads_.push_back(std::thread(func));
}

void EffectiveSink::ThreadWork(PendingLogMsg& pmsg) {
    LogMsg msg(SourceLocation{pmsg.file_name, pmsg.line, pmsg.func_name}, pmsg.level, pmsg.message);
    static thread_local MemoryBuf input_buf;
    formatter_->Format(msg, &input_buf);
    const size_t compression_bound = compress_->CompressBound(input_buf.size());
    if (compression_bound > std::numeric_limits<size_t>::max() - 16) {
        return;
    }
    const size_t encryption_bound = ((compression_bound / 16) + 1) * 16;
    if (encryption_bound > conf_.cache_payload_limit - std::min(conf_.cache_payload_limit, sizeof(Detail::ItemHead))) {
        LOG_ERROR("EffectiveSink encoded record exceeds cache payload limit");
        return;
    }
    const size_t record_bound = sizeof(Detail::ItemHead) + encryption_bound;

    while (true) {
        bool appended = false;
        bool need_post = false;
        bool need_backpressure = false;
        bool wait_for_slave = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            //主缓存满了
            if (!master_cache_->CanAppend(record_bound)) {
                if (master_cache_->Empty()) {
                    LOG_ERROR("EffectiveSink record cannot fit into an empty cache");
                    return;
                }
                //负缓存空闲，交换，设投递
                if (is_slave_free_) { 
                    is_slave_free_ = false;
                    SwapCache();
                    need_post = true;
                } else {
                    wait_for_slave = true;
                }
            //主缓存不满
            } else {
                if (master_cache_->Empty()) {
                    compress_->ResetStream();
                }
                compress_output_buf_.resize(compression_bound);
                size_t compressed_size = compress_->Compress(input_buf.data(), input_buf.size(),
                                                              compress_output_buf_.data(), compress_output_buf_.size());
                if (compressed_size == 0) {
                    LOG_ERROR("EffectiveSink::Log compress failed");
                    return ;
                }
                compress_output_buf_.resize(compressed_size);
                crypt_output_buf_.clear();
                // crypt_output_buf_.resize(compressed_size + 16);
                crypt_->Encrypt(compress_output_buf_.data(), compressed_size, crypt_output_buf_);
                if (crypt_output_buf_.empty()) {
                    LOG_ERROR("EffectiveSink::Log encrypt error");
                    return ;
                }
                //写入主缓存
                if (WriteToCache(crypt_output_buf_.data(), crypt_output_buf_.size()) != MMapAux::AppendResult::kOk) {
                    LOG_ERROR("EffectiveSink cache append failed");
                    return;
                }
                appended = true;
                //需要主从缓存交换
                if (NeedCacheToFile()) {
                    //从缓存空，交换成功
                    if (is_slave_free_) {
                        is_slave_free_ = false;
                        SwapCache();
                        need_post = true;
                    } else {
                        //从缓存还在处理，反压到入口
                        need_backpressure = true;
                    }
                }
            }
        }
        if (need_post) {
            PostSlaveTaskToWork();
        }
        if (need_backpressure) {
            SetCacheBackpressureIfSlaveBusy();
        }
        if (appended) {
            return;
        }
        if (wait_for_slave) {
            if (SetCacheBackpressureIfSlaveBusy()) {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                queue_cv_.wait(lock, [this]() { return !cache_backpressure_ || is_stop_.load(); });
            }
        }
    }
}


bool EffectiveSink::TrySwapCacheForFlush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!is_slave_free_ || master_cache_->Empty()) {
        return false;
    }
    is_slave_free_ = false;
    std::swap(master_cache_, slave_cache_);
    return true;
}


void EffectiveSink::SwapCache() {
    std::swap(master_cache_, slave_cache_);
}


MMapAux::AppendResult EffectiveSink::WriteToCache(void* data, size_t data_size) {
    Detail::ItemHead item_head;
    if (data_size > std::numeric_limits<uint32_t>::max()) {
        return MMapAux::AppendResult::kNoSpace;
    }
    item_head.size = static_cast<uint32_t>(data_size);
    return master_cache_->TryAppendRecord(&item_head, sizeof(item_head), data, data_size);
}

void EffectiveSink::CacheToFile() {
    LOG_INFO("+++++ cache to file +++++");
    std::function<void()> hook;
    bool has_data = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (is_slave_free_) {
            return ;
        }
        if (slave_cache_->Empty()) {
            is_slave_free_ = true;
        } else {
            has_data = true;
            hook = cache_write_hook_for_test_;
        }
    }
    if (!has_data) {
        ClearCacheBackpressure();
        return;
    }
    if (hook) {
        hook();
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto log_path = GetLogPath();
        if (!ofs_.is_open()) {
            ofs_.open(log_path, std::ios::binary | std::ios::app);
        }
        Detail::ChunkHead chunk_head;
        chunk_head.size = slave_cache_->Size();
        memset(chunk_head.pub_key, 0, sizeof(chunk_head.pub_key));
        memcpy(chunk_head.pub_key, self_public_key_.data(), self_public_key_.size());

        ofs_.write(reinterpret_cast<char*>(&chunk_head), sizeof(chunk_head));
        ofs_.write(reinterpret_cast<char*>(slave_cache_->Data()), slave_cache_->Size());
        ofs_.flush();

        slave_cache_->Clear();
        is_slave_free_ = true;
    }
    ClearCacheBackpressure();
}


void EffectiveSink::SetFormatter(std::unique_ptr<Formatter> formatter) {}

void EffectiveSink::Flush() {
    LOG_INFO("*** flush ***");
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        flush_cv_.wait(lock, [this]() {
            return task_queue_.empty() && active_workers_ == 0 && !encoder_busy_ && in_flight_bytes_ == 0;
        });
    }
    PostSlaveTaskToWork();
    WAIT_TASK_IDLE(task_runner_tag_);
    if (TrySwapCacheForFlush()) {
        PostSlaveTaskToWork();
        WAIT_TASK_IDLE(task_runner_tag_);
    }
}

std::filesystem::path EffectiveSink::GetLogPath() {
    auto GetTimePath = [this]() {
        auto now_time_point = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now_time_point);
        char timebuf[64] = {0};
        std::strftime(timebuf, sizeof(timebuf), "[%Y%m%d%H%M%S]", std::localtime(&t));
        return (conf_.dir / (conf_.file_name + "_" + timebuf));
    };

    if (log_file_path_.empty()) {
        log_file_path_ = GetTimePath().string() + ".log";
    } else {
        auto file_size = std::filesystem::file_size(log_file_path_);
        bytes single_size = space_cast<bytes>(conf_.single_size);
        if (file_size > single_size.count()) {
            auto file_name = GetTimePath();
            auto file_path = file_name.string() + ".log";
            ofs_.close();
            if (std::filesystem::exists(file_path)) {
                int index = 0;
                for (auto& path : std::filesystem::directory_iterator(conf_.dir)) {
                    if (path.path().filename().string().find(file_name.filename().string()) != std::string::npos) {
                        index++;
                    }
                }
                log_file_path_ = file_name.string() + "_" + std::to_string(index) + ".log";
            } else {
                log_file_path_ = file_path;
            }
        }
    }
    LOG_INFO("EffectiveSink::GetLogPath = {}", log_file_path_.string());
    return log_file_path_;
}

void EffectiveSink::EliminateTotalLogFileSize() {
    LOG_INFO("@@@@ EffectiveSink::Eliminate Log file @@@@");
    std::vector<std::filesystem::path> files;
    for (auto& p : std::filesystem::directory_iterator(conf_.dir)) {
        if (p.path().extension() == ".log") {
            files.push_back(p.path());
        }
    }
    auto compare = [](std::filesystem::path& first, std::filesystem::path& second) {
        return std::filesystem::last_write_time(first) > std::filesystem::last_write_time(second);
    };
    sort(files.begin(), files.end(), compare);

    size_t total_bytes = space_cast<bytes>(conf_.total_size).count();
    size_t count = 0;
    for (auto& file : files) {
        count += std::filesystem::file_size(file);
        if (count > total_bytes) {
            LOG_INFO("EffectiveSink::Eliminate Log File : bigger , remove {}", file.string());
            std::filesystem::remove(file);
        }
        
    }
}

void EffectiveSink::PostSlaveTaskToWork() {
    if (!POST_TASK(task_runner_tag_, [this](){CacheToFile();})) {
        CacheToFile();
    }
}

bool EffectiveSink::IsCritical(LogLevel level) const {
    return level == LogLevel::kError || level == LogLevel::kCritical;
}

bool EffectiveSink::IsAcceptedLevel(LogLevel level) const {
    return level >= LogLevel::kTrace && level <= LogLevel::kCritical;
}

bool EffectiveSink::SetCacheBackpressureIfSlaveBusy() {
    std::unique_lock<std::mutex> queue_lock(queue_mutex_);
    std::lock_guard<std::mutex> cache_lock(mutex_);
    if (is_slave_free_) {
        return false;
    }
    cache_backpressure_ = true;
    return true;
}

void EffectiveSink::ClearCacheBackpressure() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        cache_backpressure_ = false;
    }
    queue_cv_.notify_all();
}

void EffectiveSink::SetCacheWriteHookForTest(std::function<void()> hook) {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_write_hook_for_test_ = std::move(hook);
}


}
