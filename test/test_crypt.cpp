#include "crypt/aes_crypt.h"

#include <iostream>

int main() {
    std::string key = Logger::Crypt::AESCrypt::GenBinaryKey();
    std::string iv = Logger::Crypt::AESCrypt::GenBinaryIv();
    std::cout << " 二进制的key ：" << key << std::endl;
    std::cout << "16进制的key ： " << Logger::Crypt::BinaryToHex(key) << std::endl;

    Logger::Crypt::AESCrypt aes_crypt(key, iv);

    std::string input = "hello world ! aes_crypt";
    std::string output;
    aes_crypt.Encrypt(input.data(), input.size(), output);

    std::cout << " 加密后的结果  " << output << std::endl;

    std::cout << " 解密后的结果 " << aes_crypt.Decrypt(output.data(), output.size()) << std::endl;

}