#pragma once

#include <memory>
#include <string>
#include <tuple>

namespace Logger {
namespace Crypt {

class BaseCrypt {
public:
    virtual ~BaseCrypt(){}
    virtual void Encrypt(void* input, size_t input_size, std::string& output) = 0;
    virtual std::string Decrypt(void* data, size_t data_size) = 0;
};

std::tuple<std::string, std::string> GenECDHKey();

std::string GenECDHAgreeKey(std::string& self_private_key, std::string& other_public_key);

std::string BinaryToHex(std::string& binary_key);
std::string HexToBinary(std::string& hex_key);




}
}