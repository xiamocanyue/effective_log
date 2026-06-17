#pragma once 

#include "crypt/crypt.h"



namespace Logger {
namespace Crypt {

class AESCrypt : public BaseCrypt {
public:
    AESCrypt(std::string key, std::string iv);
    AESCrypt(std::string key);

    static std::string GenBinaryKey();
    static std::string GenBinaryIv();

    void Encrypt(void* input, size_t input_size, std::string& output) override;
    std::string Decrypt(void* data, size_t data_size) override;



private:
    std::string key_;
    std::string iv_;
};


}
}