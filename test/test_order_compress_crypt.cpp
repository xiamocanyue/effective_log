#include "compress/zstd_compress.h"
#include "crypt/aes_crypt.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

std::string BuildSampleLog() {
    std::string log;
    log.reserve(256 * 1024);
    for (int i = 0; i < 4000; ++i) {
        log += "2026-06-22 11:16:21.123 [INFO] [order-test] request_id=";
        log += std::to_string(i % 128);
        log += " user=alice action=flush_cache path=/var/log/effective.log status=ok message=compress then encrypt benchmark sample line\n";
    }
    return log;
}

struct BenchmarkResult {
    std::string output;
    long long total_us = 0;
};

BenchmarkResult RunCompressThenEncryptBenchmark(const std::string& input, int rounds) {
    Logger::Compress::ZstdCompression compress;
    Logger::Crypt::AESCrypt crypt(Logger::Crypt::AESCrypt::GenBinaryKey(), Logger::Crypt::AESCrypt::GenBinaryIv());

    std::string compressed;
    std::string encrypted;
    compressed.resize(compress.CompressBound(input.size()));

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < rounds; ++i) {
        compress.ResetStream();
        size_t compressed_size = compress.Compress(
            const_cast<char*>(input.data()), input.size(), compressed.data(), compressed.size());
        compressed.resize(compressed_size);
        encrypted.clear();
        crypt.Encrypt(compressed.data(), compressed.size(), encrypted);
    }
    auto end = std::chrono::steady_clock::now();

    return {
        encrypted,
        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
    };
}

BenchmarkResult RunEncryptThenCompressBenchmark(const std::string& input, int rounds) {
    Logger::Crypt::AESCrypt crypt(Logger::Crypt::AESCrypt::GenBinaryKey(), Logger::Crypt::AESCrypt::GenBinaryIv());
    Logger::Compress::ZstdCompression compress;

    std::string encrypted;
    std::string compressed;

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < rounds; ++i) {
        encrypted.clear();
        crypt.Encrypt(const_cast<char*>(input.data()), input.size(), encrypted);
        compress.ResetStream();
        compressed.resize(compress.CompressBound(encrypted.size()));
        size_t compressed_size = compress.Compress(
            encrypted.data(), encrypted.size(), compressed.data(), compressed.size());
        compressed.resize(compressed_size);
    }
    auto end = std::chrono::steady_clock::now();

    return {
        compressed,
        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
    };
}

}  // namespace

int main() {
    constexpr int kRounds = 200;
    const std::string log = BuildSampleLog();

    const BenchmarkResult compress_then_encrypt = RunCompressThenEncryptBenchmark(log, kRounds);
    const BenchmarkResult encrypt_then_compress = RunEncryptThenCompressBenchmark(log, kRounds);

    if (compress_then_encrypt.output.empty() || encrypt_then_compress.output.empty()) {
        std::cerr << "benchmark failed: empty output\n";
        return EXIT_FAILURE;
    }

    std::cout << "log size: " << log.size() << " bytes\n";
    std::cout << "rounds: " << kRounds << "\n";
    std::cout << "compress -> encrypt output size: " << compress_then_encrypt.output.size() << " bytes\n";
    std::cout << "encrypt -> compress output size: " << encrypt_then_compress.output.size() << " bytes\n";
    std::cout << "compress -> encrypt total: " << compress_then_encrypt.total_us << " us, avg: "
              << (compress_then_encrypt.total_us / static_cast<double>(kRounds)) << " us\n";
    std::cout << "encrypt -> compress total: " << encrypt_then_compress.total_us << " us, avg: "
              << (encrypt_then_compress.total_us / static_cast<double>(kRounds)) << " us\n";

    if (compress_then_encrypt.total_us <= encrypt_then_compress.total_us) {
        std::cout << "result: compress -> encrypt is faster or equal on this sample\n";
    } else {
        std::cout << "result: encrypt -> compress is faster on this sample\n";
    }

    return EXIT_SUCCESS;
}
