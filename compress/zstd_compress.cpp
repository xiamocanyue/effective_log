#include "compress/zstd_compress.h"
#include <cstring>

namespace Logger {
namespace Compress {

ZstdCompression::ZstdCompression() : cctx_(ZSTD_createCCtx()), dctx_(ZSTD_createDCtx()) {
    ZSTD_CCtx_setParameter(cctx_.get(), ZSTD_c_compressionLevel, 5);
}

bool IsNewCompress(void* data, size_t data_size) {
    if (!data || data_size < 4) {
        return false;
    }
    uint8_t* check_magic = static_cast<uint8_t*>(data);
    uint8_t target_magic_big_end[] = {0x28, 0xb5, 0x2f, 0xfd};
    uint8_t target_magic_small_end[] = {0xfd, 0x2f, 0xb5, 0x28};
    if (memcmp(check_magic, target_magic_big_end, sizeof(target_magic_big_end)) == 0) {
        return true;
    }
    if (memcmp(check_magic, target_magic_small_end, sizeof(target_magic_small_end)) == 0) {
        return true;
    }
    return false;
}

size_t ZstdCompression::Compress(void* input, size_t input_size, void* output, size_t output_size) {
    if (input_size == 0 || !input) {
        return 0;
    }
    ZSTD_inBuffer zstd_input = {input, input_size, 0};
    ZSTD_outBuffer zstd_output = {output, output_size, 0};

    size_t ret = ZSTD_compressStream2(cctx_.get(), &zstd_output, &zstd_input, ZSTD_e_flush);
    if (ZSTD_isError(ret) != 0) {
        return 0;
    }
    return zstd_output.pos;
}

std::string ZstdCompression::DeCompress(void* data, size_t data_size) {
    if (data_size == 0 || !data) {
        return "";
    }
    if (IsNewCompress(data, data_size)) {
        ResetDCtx();
    }
    ZSTD_inBuffer zstd_input = {data, data_size, 0};
    std::string output;
    output.reserve(1024 * 10);
    ZSTD_outBuffer zstd_output = {static_cast<void*>(output.data()), output.capacity(), 0};

    size_t ret = ZSTD_decompressStream(dctx_.get(), &zstd_output, &zstd_input);
    if (ZSTD_isError(ret) != 0) {
        return "";
    }
    output = std::string(static_cast<char*>(zstd_output.dst), zstd_output.pos);
    return output;
}

size_t ZstdCompression::CompressBound(size_t input_size) {
    return ZSTD_compressBound(input_size);
}

void ZstdCompression::ResetStream() {
    ZSTD_CCtx_reset(cctx_.get(), ZSTD_reset_session_only);
}

void ZstdCompression::ResetDCtx() {
    ZSTD_DCtx_reset(dctx_.get(), ZSTD_reset_session_only);
}


}
}