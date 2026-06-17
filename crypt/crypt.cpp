#include "crypt/crypt.h"

#include <cryptopp/aes.h>
#include <cryptopp/base64.h>
#include <cryptopp/cryptlib.h>
#include <cryptopp/eccrypto.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/modes.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>

#include <stdexcept>

namespace Logger {
namespace Crypt {

std::tuple<std::string, std::string> GenECDHKey() {
    CryptoPP::AutoSeededRandomPool rnd;
    CryptoPP::ECDH<CryptoPP::ECP>::Domain do_ecdh_main(CryptoPP::ASN1::secp256r1());
    CryptoPP::SecByteBlock private_key(do_ecdh_main.PrivateKeyLength());
    CryptoPP::SecByteBlock public_key(do_ecdh_main.PublicKeyLength());
    do_ecdh_main.GenerateKeyPair(rnd, private_key, public_key);
    return std::make_tuple(std::string(reinterpret_cast<char*>(private_key.data()), private_key.size()), std::string(reinterpret_cast<char*>(public_key.data()), public_key.size()));
}

std::string GenECDHAgreeKey(std::string& self_private_key, std::string& other_public_key) {
    CryptoPP::AutoSeededRandomPool rnd;
    CryptoPP::ECDH<CryptoPP::ECP>::Domain do_ecdh_main(CryptoPP::ASN1::secp256r1());
    CryptoPP::SecByteBlock agree_key(do_ecdh_main.AgreedValueLength());
    CryptoPP::SecByteBlock pri(reinterpret_cast<CryptoPP::byte*>(self_private_key.data()), self_private_key.size());
    CryptoPP::SecByteBlock pub(reinterpret_cast<CryptoPP::byte*>(other_public_key.data()), other_public_key.size());
    if (!do_ecdh_main.Agree(agree_key, pri, pub)) {
        throw std::runtime_error("fail to get agree key");
    }

    CryptoPP::SHA256 hash;
    CryptoPP::SecByteBlock aes_256_key(CryptoPP::SHA256::DIGESTSIZE);
    hash.CalculateDigest(aes_256_key.data(), agree_key.data(), agree_key.size());

    return std::string(reinterpret_cast<char*>(aes_256_key.data()), aes_256_key.size());
}

std::string BinaryToHex(std::string& binary_key) {
    std::string hex_key;
    CryptoPP::StringSource ss(binary_key, true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(hex_key), false));
    return hex_key;
}

std::string HexToBinary(std::string& hex_key) {
    std::string binary_key;
    CryptoPP::StringSource ss(hex_key, true, new CryptoPP::HexDecoder(new CryptoPP::StringSink(binary_key)));
    return binary_key;
}


}
} 
