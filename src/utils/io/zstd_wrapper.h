//
// Created by igor on 29/07/2021.
//

#ifndef NEUTRINO_ZSTD_WRAPPER_H
#define NEUTRINO_ZSTD_WRAPPER_H

#include <thirdparty/zlib/zlib.h>

#if defined(__cplusplus)
extern "C" {
#endif

    int zstd_deflate_init (z_streamp strm, int level, const char *version, int stream_size);
    int zstd_deflate_init2 (z_streamp strm, int level, int method, int windowBits, int memLevel,  int strategy, const char *version, int stream_size);
    int zstd_deflate_reset (z_streamp strm);
    int zstd_deflate_set_dictionary (z_streamp strm, const Bytef *dictionary, uInt  dictLength);
    int zstd_deflate (z_streamp strm, int flush);
    int zstd_deflate_end (z_streamp strm);
    uLong zstd_deflate_bound (z_streamp strm, uLong sourceLen);
    int zstd_deflate_params (z_streamp strm, int level, int strategy);
    int zstd_inflate_init (z_streamp strm, const char* version, int stream_size);
    int zstd_inflate_init2 (z_streamp strm, int  windowBits, const char *version, int stream_size);
    int zstd_inflate_reset (z_streamp strm);
    int zstd_inflate_reset_2 (z_streamp strm, int windowBits);
    int zstd_inflate_set_dictionary (z_streamp strm, const Bytef *dictionary, uInt  dictLength);
    int zstd_inflate (z_streamp strm, int flush);
    int zstd_inflate_end (z_streamp strm);
    int zstd_inflate_sync (z_streamp strm);
    int zstd_deflate_copy (z_streamp dest, z_streamp source);
    int zstd_deflate_tune (z_streamp strm, int good_length, int max_lazy, int nice_length, int max_chain);
    int zstd_deflate_pending (z_streamp strm, unsigned *pending, int *bits);
    int zstd_deflate_prime (z_streamp strm, int bits, int value);
    int zstd_deflate_set_header (z_streamp strm, gz_headerp head);
    int zstd_inflate_get_dictionary (z_streamp strm, Bytef *dictionary, uInt  *dictLength);
    int zstd_inflate_copy (z_streamp dest, z_streamp source);
    long zstd_inflate_mark (z_streamp strm);
    int zstd_inflate_prime (z_streamp strm, int bits, int value);
    int zstd_inflate_get_header (z_streamp strm, gz_headerp head);
    int zstd_inflate_back_init (z_streamp strm, int windowBits, unsigned char FAR *window, const char *version, int stream_size);
    int zstd_inflate_back (z_streamp strm, in_func in, void FAR *in_desc, out_func out, void FAR *out_desc);
    int zstd_inflate_back_end (z_streamp strm);
    int zstd_compress (Bytef * dest, uLongf * destLen, const Bytef *source, uLong sourceLen);
    int zstd_compress2 (Bytef * dest, uLongf * destLen, const Bytef *source, uLong sourceLen, int level);
    uLong zstd_compress_bound (uLong sourceLen);
    int zstd_uncompress (Bytef * dest, uLongf * destLen, const Bytef *source, uLong sourceLen);
    uLong zstd_adler32 (uLong adler, const Bytef *buf, uInt len);
    uLong zstd_crc32   (uLong crc, const Bytef *buf, uInt len);
    uLong zstd_adler32_z (uLong adler, const Bytef *buf, z_size_t len);
    uLong zstd_crc32_z (uLong crc, const Bytef *buf, z_size_t len);
    const z_crc_t FAR * zstd_get_crc_table    (void);
    const char * zstd_zError (int err);

#if defined(__cplusplus)
}
#endif

#endif //NEUTRINO_ZSTD_WRAPPER_H
