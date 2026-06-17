#include "sinks/effective_sink.h"

#include "crypt/aes_crypt.h"
#include "compress/zstd_compress.h"
#include "internal_log.h"

#include <fstream>
#include <vector>

namespace Logger {

EffectiveSink::EffectiveSink(Conf conf) : conf_(std::move(conf)) {
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
    master_cache_ = std::make_unique<MMapAux>("master_cache");
    slave_cache_ = std::make_unique<MMapAux>("slave_cache");

    if (!slave_cache_->Empty()) {
        is_slave_free_.store(false);
        PostSlaveTaskToWork();
        WAIT_TASK_IDLE(task_runner_tag_);
    }
    if (!master_cache_->Empty()) {
        if (is_slave_free_.load()) {
            is_slave_free_.store(false);
            SwapCache();
        }
        PostSlaveTaskToWork();
    }
    POST_REPEATED_TASK(-1, conf_.passed_time_interval, task_runner_tag_, [this]() {EliminateTotalLogFileSize();});
}


bool EffectiveSink::NeedCacheToFile() {
    return master_cache_->GetRatio() > 0.8;
}

void EffectiveSink::Log(LogMsg& msg) {
    static thread_local MemoryBuf input_buf;
    formatter_->Format(msg, &input_buf);

    {
        if (master_cache_->Empty()) {
            compress_->ResetStream();
        }
        std::lock_guard<std::mutex> lock(mutex_);
        compress_output_buf_.resize(compress_->CompressBound(input_buf.size()));
        size_t compressed_size = compress_->Compress(input_buf.data(), input_buf.size(), compress_output_buf_.data(), compress_output_buf_.size());
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
        WriteToCache(crypt_output_buf_.data(), crypt_output_buf_.size());
    }
    if (NeedCacheToFile()) {
        if (is_slave_free_.load()) {
            is_slave_free_.store(false);
            SwapCache();
        }
        PostSlaveTaskToWork();
    }
}

void EffectiveSink::SwapCache() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::swap(master_cache_, slave_cache_);
}


void EffectiveSink::WriteToCache(void* data, size_t data_size) {
    Detail::ItemHead item_head;
    item_head.size = data_size;
    master_cache_->Push(&item_head, sizeof(item_head));
    master_cache_->Push(data, data_size);
}

void EffectiveSink::CacheToFile() {
    LOG_INFO("+++++ cache to file +++++");
    if (is_slave_free_.load()) {
        return ;
    }
    if (slave_cache_->Empty()) {
        is_slave_free_.store(true);
        return ;
    }
    auto log_file_path = GetLogPath();
    Detail::ChunkHead chunk_head;
    chunk_head.size = slave_cache_->Size();
    memset(chunk_head.pub_key, 0, sizeof(chunk_head.pub_key));
    memcpy(chunk_head.pub_key, self_public_key_.data(), self_public_key_.size());
    
    std::ofstream ofs(log_file_path, std::ios::binary | std::ios::app);
    ofs.write(reinterpret_cast<char*>(&chunk_head), sizeof(chunk_head));
    ofs.write(reinterpret_cast<char*>(slave_cache_->Data()), slave_cache_->Size());
    ofs.close();

    slave_cache_->Clear();
    is_slave_free_.store(true);
}


void EffectiveSink::SetFormatter(std::unique_ptr<Formatter> formatter) {}

void EffectiveSink::Flush() {
    LOG_INFO("*** flush ***");
    PostSlaveTaskToWork();
    WAIT_TASK_IDLE(task_runner_tag_);
    if (is_slave_free_.load()) {
        is_slave_free_.store(false);
        SwapCache();
    }
    PostSlaveTaskToWork();
    WAIT_TASK_IDLE(task_runner_tag_);
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
    POST_TASK(task_runner_tag_, [this](){CacheToFile();});
}





}
