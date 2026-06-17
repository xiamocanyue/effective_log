#pragma once 


#include "compress/compress.h"
#include <zstd.h>
#include <memory>

namespace Logger {
namespace Compress {

struct ZstdCCtxDeleter {
    void operator()(ZSTD_CCtx* cctx) {
        if (cctx) {
            ZSTD_freeCCtx(cctx);
        }
    }
};

struct ZstdDCtxDeleter {
    void operator()(ZSTD_DCtx* dctx) {
        if (dctx) {
            ZSTD_freeDCtx(dctx);
        }
    }
};

class ZstdCompression : public Compression {
public:
    ZstdCompression();
    size_t Compress(void* input, size_t input_size, void* output, size_t output_size) override;
    std::string DeCompress(void* data, size_t data_size) override;

    size_t CompressBound(size_t input_size) override;
    void ResetStream() override;

private:
    void ResetDCtx();

private:
    std::unique_ptr<ZSTD_CCtx, ZstdCCtxDeleter> cctx_;
    std::unique_ptr<ZSTD_DCtx, ZstdDCtxDeleter> dctx_;

};
}
}