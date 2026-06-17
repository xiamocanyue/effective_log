#include "compress/zstd_compress.h"
#include "compress/zlib_compress.h"

#include <iostream>

int main() {
    Logger::Compress::ZlibCompression zlib_cmp;
    std::string input_zlib = "hhhhhhhhhhhhhhhhhhhhhhhhhhhelowhellow world zlib";
    std::string output_zlib;
    size_t suit_size = zlib_cmp.CompressBound(input_zlib.size());
    output_zlib.resize(suit_size);
    size_t output_zlib_size = zlib_cmp.Compress(input_zlib.data(), input_zlib.size(), output_zlib.data(), output_zlib.size());
    output_zlib.resize(output_zlib_size);

    std::cout << input_zlib << "  :初始的大小为: " << input_zlib.size() << std::endl;
    std::cout << output_zlib << "   :压缩后的大小为: " << output_zlib.size() << std::endl;
    std::cout << "decompress : " << zlib_cmp.DeCompress(output_zlib.data(), output_zlib.size()) << std::endl;
    



    Logger::Compress::ZstdCompression zstd_cmp;
    std::string input_zstd = "hellow world zstdhellow world zstdhellow world zstdhellow world zstdhellow world zstd";
    std::string output_zstd;
    suit_size = zstd_cmp.CompressBound(input_zstd.size());
    output_zstd.resize(suit_size);
    size_t output_zstd_size = zstd_cmp.Compress(input_zstd.data(), input_zstd.size(), output_zstd.data(), output_zstd.size());
    output_zstd.resize(output_zstd_size);
    std::cout << input_zstd << "   :初始的大小为: " << input_zstd.size() << std::endl;
    std::cout << output_zstd << "   :压缩后的大小为: " << output_zstd.size() << std::endl;
    std::cout << "decompress : " << zstd_cmp.DeCompress(output_zstd.data(), output_zstd.size()) << std::endl;
   
    return 0;
}