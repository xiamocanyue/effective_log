#include "sinks/effective_sink.h"
#include "log_handle.h"
#include "log_variadic_handle.h"
#include "log_factory.h"
#include "crypt/crypt.h"
#include "compress/zstd_compress.h"
#include "read_log/decode_log.h"

#include <iostream>
#include <memory>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <vector>

namespace fs = std::filesystem;

// 生成一对 ECDH 密钥
static std::pair<std::string, std::string> GenTestKeyPair() {
    auto [priv, pub] = Logger::Crypt::GenECDHKey();
    std::string pub_hex = Logger::Crypt::BinaryToHex(pub);
    return {priv, pub_hex};
}

static void PrintPass(const char* test_name) {
    std::cout << "[PASS] " << test_name << "\n";
}

static void PrintFail(const char* test_name, const char* reason) {
    std::cout << "[FAIL] " << test_name << " : " << reason << "\n";
}

static fs::path PrepareTestDir() {
    auto dir = fs::path("test_output") / "basic_effective_log";
    fs::create_directories(dir);
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() == ".log") {
            fs::remove(entry.path());
        }
    }
    return dir;
}

static fs::path FindLatestLogFile(const fs::path& dir) {
    fs::path latest;
    auto latest_time = fs::file_time_type::min();
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() != ".log") {
            continue;
        }
        auto write_time = fs::last_write_time(entry.path());
        if (latest.empty() || write_time > latest_time) {
            latest = entry.path();
            latest_time = write_time;
        }
    }
    return latest;
}

// 写入日志 -> 落盘 -> 解密 -> 终端输出
void TestEndToEndFlow() {
    const char* name = "TestEndToEndFlow";
    try {
        auto log_dir = PrepareTestDir();
        auto [other_private_key, other_pub_hex] = GenTestKeyPair();

        Logger::EffectiveSink::Conf conf;
        conf.other_public_key = other_pub_hex;
        conf.dir = log_dir;
        conf.file_name = "basic_effective_log";
        conf.passed_time_interval = std::chrono::minutes(1);

        auto sink = std::make_shared<Logger::EffectiveSink>(conf);
        Logger::LogHandle handle(sink);

        handle.CreateLogMsg(Logger::SourceLocation(__FILE__, __LINE__, __func__), Logger::LogLevel::kWarn, "warn log for end to end test");
        handle.CreateLogMsg(Logger::SourceLocation(__FILE__, __LINE__, __func__), Logger::LogLevel::kError, "error log for end to end test");

        sink->Flush();

        auto log_path = FindLatestLogFile(log_dir);
        if (log_path.empty() || !fs::exists(log_path)) {
            PrintFail(name, "log file does not exist after Flush()");
            return;
        }

        auto file_size = fs::file_size(log_path);
        if (file_size == 0) {
            PrintFail(name, "log file is empty after Flush()");
            return;
        }

        std::cout << "  log file: " << log_path.string() << "\n";
        std::cout << "  log file size: " << file_size << " bytes\n";

        decompress = std::make_unique<Logger::Compress::ZstdCompression>();
        std::string decoded_output;
        std::string log_path_string = log_path.string();
        DecodeLog(log_path_string, other_private_key, decoded_output);

        if (!decoded_output.empty()) {
            std::cout << "=== decoded log ===\n";
            std::cout << decoded_output;
        }

        PrintPass(name);
    } catch (const std::exception& e) {
        PrintFail(name, e.what());
    }
}

int main() {
    std::cout << "=== test_basic_effective_log ===\n";
    TestEndToEndFlow();
    int arr[10] = {0};
    for (int i = 0; i < 10; i++) {
        std::cout << arr[i];
    }
    std::cout << arr[-1] << "cecc" << std::endl;

    return 0;
}
