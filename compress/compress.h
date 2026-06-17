#pragma once

#include <stdio.h>
#include <string>

namespace Logger {
namespace Compress {

class Compression {
public:
    virtual ~Compression(){}

    virtual size_t Compress(void* input, size_t input_size, void* output, size_t output_size) = 0;
    virtual std::string DeCompress(void* data, size_t data_size) = 0;
    
    virtual size_t CompressBound(size_t input_size) = 0;
    virtual void ResetStream() = 0;

};

}
}

