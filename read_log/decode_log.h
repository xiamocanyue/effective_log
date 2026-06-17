#pragma once 

#include "crypt/crypt.h"
#include "compress/compress.h"
#include "read_log/read_log_formatter.h"
#include "sinks/effective_sink.h"
#include "crypt/aes_crypt.h"
#include "formatter/default_formatter.h"

#include <string>
#include <fstream>
#include <filesystem>
#include <vector>

std::unique_ptr<Logger::Compress::Compression> decompress;


std::vector<char> ReadFile(std::string& file_path) {
    std::ifstream ifs(file_path, std::ios::binary);
    ifs.seekg(0, std::ios::end);
    auto file_size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::vector<char> buf(file_size);
    ifs.read(buf.data(), file_size);
    return buf;
}

void DefaultDecodeFormat(EffectiveMsg& msg) {
    Logger::LogMsg log_msg(Logger::SourceLocation{msg.file_name(), msg.line(), msg.func_name()}, static_cast<Logger::LogLevel>(msg.level()), msg.log_info());
    Logger::DefaultFormatter defaultformat;
    Logger::MemoryBuf buf;
    defaultformat.Format(log_msg, &buf);
    std::cout << buf << std::endl;

}

void DecodeItemData(const char* input, size_t data_size, Logger::Crypt::BaseCrypt* crypt, std::string& output_buf) {
    std::string decrypted = crypt->Decrypt(const_cast<char*>(input), data_size);
    std::string decompressed = decompress->DeCompress(decrypted.data(), decrypted.size());
    EffectiveMsg msg;
    msg.ParseFromString(decompressed);
    DefaultDecodeFormat(msg);
}

void DecodeChunkData(const char* input, size_t input_size, const std::string& other_public_key, std::string& self_private_key, std::string& output_buf) {
    std::string other_public_key_bin = Logger::Crypt::HexToBinary(const_cast<std::string&>(other_public_key));
    std::string shared_key = Logger::Crypt::GenECDHAgreeKey(self_private_key, other_public_key_bin);
    std::unique_ptr<Logger::Crypt::BaseCrypt> crypt = std::make_unique<Logger::Crypt::AESCrypt>(shared_key);

    size_t offset = 0;
    size_t count = 0;
    while (offset < input_size) {
        ++count;
        auto item_head = reinterpret_cast<Logger::Detail::ItemHead*>(const_cast<char*>(input) + offset);
        if (item_head->magic != Logger::Detail::ItemHead::kMagic) {
            throw std::runtime_error("DecodeChunkData: item_head magic is not ok");
            return ;
        }
        offset += sizeof(Logger::Detail::ItemHead);
        DecodeItemData(input + offset, item_head->size, crypt.get(), output_buf);
        offset += item_head->size;
        output_buf.push_back('\n');
    }
}


void DecodeLog(std::string& inpute_file_path, std::string& private_key, std::string& output_buf) {
    auto file_buf = ReadFile(inpute_file_path);
    if (file_buf.size() < sizeof(Logger::Detail::ChunkHead)) {
        throw std::runtime_error("DecodeFile : input files is small");
        return ;
    }
    auto chunk_head = reinterpret_cast<Logger::Detail::ChunkHead*>(file_buf.data());
    if (chunk_head->magic != Logger::Detail::ChunkHead::kMagic) {
        throw std::runtime_error("DecodeFile : magic is not OK");
        return ;
    }

    size_t offset = 0;
    size_t file_buf_size = file_buf.size();
    output_buf.reserve(1024 * 1024);
    while (offset < file_buf_size) {
        Logger::Detail::ChunkHead* chunk_head = reinterpret_cast<Logger::Detail::ChunkHead*>(file_buf.data() + offset);
        if (chunk_head->magic != Logger::Detail::ChunkHead::kMagic) {
            throw std::runtime_error("Decode : in while, chunk_head magic is not ok");
            return ;
        }
        output_buf.clear();
        offset += sizeof(Logger::Detail::ChunkHead);
        DecodeChunkData(file_buf.data() + offset, chunk_head->size, std::string(chunk_head->pub_key), private_key, output_buf);
        offset += chunk_head->size;
    }



}
