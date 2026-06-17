#include "crypt/aes_crypt.h"

#include <cryptopp/aes.h>
#include <cryptopp/base64.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/modes.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>


namespace Logger {
namespace Crypt {

std::string AESCrypt::GenBinaryKey() {
    CryptoPP::AutoSeededRandomPool rnd;
    CryptoPP::SecByteBlock key(CryptoPP::AES::DEFAULT_KEYLENGTH);
    rnd.GenerateBlock(key.data(), key.size());
    return std::string(reinterpret_cast<char*>(key.data()), key.size());
}

std::string AESCrypt::GenBinaryIv() {
    CryptoPP::AutoSeededRandomPool rnd;
    CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
    rnd.GenerateBlock(iv.data(), iv.size());
    return std::string(reinterpret_cast<char*>(iv.data()), iv.size());
}

AESCrypt::AESCrypt(std::string key, std::string iv) : key_(std::move(key)), iv_(std::move(iv)) {}

AESCrypt::AESCrypt(std::string key) : key_(std::move(key)) {
    iv_ = "dad0c0012340080a";
}

void AESCrypt::Encrypt(void* input, size_t input_size, std::string& output) {
    CryptoPP::AES::Encryption aes_encrypt(reinterpret_cast<CryptoPP::byte*>(key_.data()), key_.size());
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbc_encrypt(aes_encrypt, reinterpret_cast<CryptoPP::byte*>(iv_.data()));
    CryptoPP::StreamTransformationFilter str_filter(cbc_encrypt, new CryptoPP::StringSink(output));
    str_filter.Put(reinterpret_cast<CryptoPP::byte*>(input), input_size);
    str_filter.MessageEnd();
}

std::string AESCrypt::Decrypt(void* data, size_t data_size) {
    CryptoPP::AES::Decryption aes_decrypt(reinterpret_cast<CryptoPP::byte*>(key_.data()), key_.size());
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbc_decrypt(aes_decrypt, reinterpret_cast<CryptoPP::byte*>(iv_.data()));
    std::string output;
    CryptoPP::StreamTransformationFilter str_filter(cbc_decrypt, new CryptoPP::StringSink(output));
    str_filter.Put(reinterpret_cast<CryptoPP::byte*>(data), data_size);
    str_filter.MessageEnd();
    return output;
}


}
}