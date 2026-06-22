#include "compress/zlib_compress.h"


namespace Logger {
namespace Compress {

ZlibCompression::ZlibCompression() {
    ResetStream();
    ResetDeCompressStream();
}

bool IsNewCompressed(void* data, size_t data_size)  {
    if (!data) {
        return false;
    }
    if (data_size < 2) {
        return false;
    }

    uint8_t* check_magic = static_cast<uint8_t*>(data);
    if (check_magic[0] == 0x78 && (check_magic[1] == 0x9c || check_magic[1] == 0xda || check_magic[1] == 0x01 || check_magic[1] == 0x5e)) {
        return true;
    }
    return false;
}

size_t ZlibCompression::Compress(void* input, size_t input_size, void* output, size_t output_size){
    if (!input || !output) {
        return 0;
    }
    if (!compress_strm_) {
        ResetStream();
        if (!compress_strm_) {
            return 0;
        }
    }

    compress_strm_->next_in = static_cast<Bytef*>(input);
    compress_strm_->avail_in = static_cast<uInt>(input_size);
    compress_strm_->next_out = static_cast<Bytef*>(output);
    compress_strm_->avail_out = static_cast<uInt>(output_size);

    int ret = deflate(compress_strm_.get(), Z_SYNC_FLUSH);
    if (ret != Z_OK && ret != Z_BUF_ERROR && ret != Z_STREAM_END) {
        return 0;
    }
    if (compress_strm_->avail_in != 0) {
        return 0;
    }
    size_t out_len = output_size - compress_strm_->avail_out;
    return out_len;
}

std::string ZlibCompression::DeCompress(void* data, size_t data_size) {
    if (!data) {
        return "";
    }

    if (IsNewCompressed(data, data_size)) {
        ResetDeCompressStream();
    }

    if (!decompress_strm_) {
        ResetDeCompressStream();

        if (!decompress_strm_) {
        return "";
    }
    }

    decompress_strm_->next_in = static_cast<Bytef*>(data);
    decompress_strm_->avail_in = static_cast<uInt>(data_size);

    std::string output;
    while (decompress_strm_->avail_in > 0) {
        char buf[4096] = {0};
        decompress_strm_->next_out = reinterpret_cast<Bytef*>(const_cast<char*>(buf));
        decompress_strm_->avail_out = sizeof(buf);

        auto ret = inflate(decompress_strm_.get(), Z_SYNC_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            return "";
        }
        output.append(buf, sizeof(buf) - decompress_strm_->avail_out);
        if (ret == Z_STREAM_END) {
            break;
        }
    }
    return output;
}

size_t ZlibCompression::CompressBound(size_t input_size) {
    if (!compress_strm_) {
        ResetStream();
        if (!compress_strm_) {
            return 0;
        }
    }
    return deflateBound(compress_strm_.get(), input_size);
}

void ZlibCompression::ResetStream() {
    compress_strm_.reset(new z_stream());

    compress_strm_->zalloc = Z_NULL;
    compress_strm_->zfree = Z_NULL;
    compress_strm_->opaque = Z_NULL;

    int ret = deflateInit2(compress_strm_.get(), Z_BEST_COMPRESSION, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY);

    if (ret != Z_OK) {
        compress_strm_.reset();
    }

}

void ZlibCompression::ResetDeCompressStream() {
    decompress_strm_.reset(new z_stream());
    decompress_strm_->zalloc = Z_NULL;
    decompress_strm_->zfree = Z_NULL;
    decompress_strm_->opaque = Z_NULL;
    decompress_strm_->avail_in = 0;
    decompress_strm_->next_in = Z_NULL;

    int ret = inflateInit2(decompress_strm_.get(), 15);
    if (ret != Z_OK) {
        decompress_strm_.reset();
    }
}



}
}