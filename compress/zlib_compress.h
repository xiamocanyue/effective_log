#pragma once 

#include "compress.h" 

#include <zlib.h>
#include <memory>


namespace Logger {
namespace Compress {

struct ZlibDeflateDeleter {
    void operator()(z_stream* strm) {
        if (strm) {
            deflateEnd(strm);
            delete strm;
        }
    }
};

struct ZlibInflateDeleter {
    void operator()(z_stream* strm) {
        if (strm) {
            inflateEnd(strm);
            delete strm;
        }
    }
};

class ZlibCompression : public Compression {
public:
    ZlibCompression();

    size_t Compress(void* input, size_t input_size, void* output, size_t output_size) override;
    std::string DeCompress(void* data, size_t data_size) override;

    size_t CompressBound(size_t input_size) override;
    void ResetStream() override;

private:
    void ResetDeCompressStream();

private:
    std::unique_ptr<z_stream, ZlibDeflateDeleter> compress_strm_;
    std::unique_ptr<z_stream, ZlibInflateDeleter> decompress_strm_;

};

    
}
}