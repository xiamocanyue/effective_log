#include "sinks/effective_sink.h"
#include "formatter/effective_formatter.h"
#include "crypt/aes_crypt.h"
#include "compress/zstd_compress.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace {

using Clock = std::chrono::steady_clock;

struct QueueBenchmarkResult {
    long long submit_us = 0;
    long long total_us = 0;
    size_t log_file_size = 0;
};

struct DirectBenchmarkResult {
    long long total_us = 0;
    size_t total_output_size = 0;
};

std::pair<std::string, std::string> GenTestKeyPair() {
    auto [priv, pub] = Logger::Crypt::GenECDHKey();
    std::string pub_hex = Logger::Crypt::BinaryToHex(pub);
    return {priv, pub_hex};
}

fs::path PrepareTestDir(const std::string& name) {
    fs::path dir = fs::path("test_output") / name;
    fs::create_directories(dir);
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".log") {
            fs::remove(entry.path());
        }
    }
    return dir;
}

fs::path FindLatestLogFile(const fs::path& dir) {
    fs::path latest;
    auto latest_time = fs::file_time_type::min();
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() != ".log") {
            continue;
        }
        const auto write_time = fs::last_write_time(entry.path());
        if (latest.empty() || write_time > latest_time) {
            latest = entry.path();
            latest_time = write_time;
        }
    }
    return latest;
}

std::vector<std::string> BuildMessages(size_t count) {
    std::vector<std::string> messages;
    messages.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        messages.push_back(
            "queue benchmark log index=" + std::to_string(i) +
            " user=alice trace_id=" + std::to_string(i % 512) +
            " path=/api/v1/log/report action=compress_encrypt status=200 payload=abcdefghijklmnopqrstuvwxyz0123456789 repeated repeated repeated repeated");
    }
    return messages;
}

QueueBenchmarkResult BenchmarkQueuePath(const std::vector<std::string>& messages) {
    const auto [other_private_key, other_pub_hex] = GenTestKeyPair();
    (void)other_private_key;
    const fs::path log_dir = PrepareTestDir("effective_sink_queue_benchmark");

    Logger::EffectiveSink::Conf conf;
    conf.other_public_key = other_pub_hex;
    conf.dir = log_dir;
    conf.file_name = "queue_benchmark";
    conf.passed_time_interval = std::chrono::minutes(10);

    auto sink = std::make_shared<Logger::EffectiveSink>(conf);

    const auto submit_start = Clock::now();
    for (size_t i = 0; i < messages.size(); ++i) {
        Logger::LogMsg msg(
            Logger::SourceLocation(__FILE__, __LINE__, __func__),
            Logger::LogLevel::kInfo,
            messages[i]);
        sink->Log(msg);
    }
    const auto submit_end = Clock::now();

    sink->Flush();
    const auto total_end = Clock::now();

    size_t log_file_size = 0;
    const fs::path log_file = FindLatestLogFile(log_dir);
    if (!log_file.empty() && fs::exists(log_file)) {
        log_file_size = fs::file_size(log_file);
    }

    return {
        std::chrono::duration_cast<std::chrono::microseconds>(submit_end - submit_start).count(),
        std::chrono::duration_cast<std::chrono::microseconds>(total_end - submit_start).count(),
        log_file_size,
    };
}

DirectBenchmarkResult BenchmarkDirectPath(const std::vector<std::string>& messages) {
    auto [self_private_key, self_public_key] = Logger::Crypt::GenECDHKey();
    auto [other_private_key, other_public_key] = Logger::Crypt::GenECDHKey();
    (void)self_public_key;
    (void)other_private_key;
    std::string aes_shared_key = Logger::Crypt::GenECDHAgreeKey(self_private_key, other_public_key);

    Logger::EffectiveFormatter formatter;
    Logger::Compress::ZstdCompression compress;
    Logger::Crypt::AESCrypt crypt(aes_shared_key);
    Logger::MemoryBuf formatted;
    std::string compressed;
    std::string encrypted;
    size_t total_output_size = 0;

    compress.ResetStream();
    const auto start = Clock::now();
    for (size_t i = 0; i < messages.size(); ++i) {
        Logger::LogMsg msg(
            Logger::SourceLocation(__FILE__, __LINE__, __func__),
            Logger::LogLevel::kInfo,
            messages[i]);
        formatter.Format(msg, &formatted);

        compressed.resize(compress.CompressBound(formatted.size()));
        const size_t compressed_size = compress.Compress(
            formatted.data(), formatted.size(), compressed.data(), compressed.size());
        compressed.resize(compressed_size);

        encrypted.clear();
        crypt.Encrypt(compressed.data(), compressed.size(), encrypted);
        total_output_size += encrypted.size();
    }
    const auto end = Clock::now();

    return {
        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count(),
        total_output_size,
    };
}

double ToLogsPerSecond(size_t count, long long total_us) {
    if (total_us == 0) {
        return 0.0;
    }
    return static_cast<double>(count) * 1000000.0 / static_cast<double>(total_us);
}

}  // namespace

int main() {
    constexpr size_t kMessageCount = 10000;
    const std::vector<std::string> messages = BuildMessages(kMessageCount);

    const QueueBenchmarkResult queue_result = BenchmarkQueuePath(messages);
    const DirectBenchmarkResult direct_result = BenchmarkDirectPath(messages);

    std::cout << "message count: " << kMessageCount << "\n";
    std::cout << "queue submit time: " << queue_result.submit_us << " us\n";
    std::cout << "queue submit throughput: " << ToLogsPerSecond(kMessageCount, queue_result.submit_us) << " logs/s\n";
    std::cout << "queue total time (submit + flush): " << queue_result.total_us << " us\n";
    std::cout << "queue end-to-end throughput: " << ToLogsPerSecond(kMessageCount, queue_result.total_us) << " logs/s\n";
    std::cout << "queue log file size: " << queue_result.log_file_size << " bytes\n";
    std::cout << "direct inline time: " << direct_result.total_us << " us\n";
    std::cout << "direct inline throughput: " << ToLogsPerSecond(kMessageCount, direct_result.total_us) << " logs/s\n";
    std::cout << "direct total encrypted size: " << direct_result.total_output_size << " bytes\n";

    if (queue_result.submit_us > 0) {
        std::cout << "sender thread speedup (direct / queue-submit): "
                  << (static_cast<double>(direct_result.total_us) / static_cast<double>(queue_result.submit_us))
                  << "x\n";
    }
    if (queue_result.total_us > 0) {
        std::cout << "end-to-end ratio (direct / queue-total): "
                  << (static_cast<double>(direct_result.total_us) / static_cast<double>(queue_result.total_us))
                  << "x\n";
    }

    return 0;
}
