//
// Created by igor on 24/04/2022.
//

#include "compression_stream.hh"
#include <neutrino/utils/exception.hh>

#include "libz_stream.hh"
#include "bzip_stream.hh"
#include "lzma_stream.hh"


namespace neutrino::utils::io {
  compression_stream_base::~compression_stream_base () = default;

  std::unique_ptr<compression_stream> compression_stream::create(compression_type_t type,
                                                                 compression_level_t level) {
    switch (type) {
      case compression_type_t::ZLIB:
        return std::make_unique<libz_compressor>(libz_kind_t::ZLIB, level);
      case compression_type_t::GZIP:
        return std::make_unique<libz_compressor>(libz_kind_t::GZIP, level);
      case compression_type_t::ZLIB_RAW:
        return std::make_unique<libz_compressor>(libz_kind_t::RAW, level);
      case compression_type_t::ZSTD:
        return std::make_unique<libz_compressor>(libz_kind_t::ZSTD, level);
      case compression_type_t::BZ2:
        return std::make_unique<bzip_compressor>(level);
      case compression_type_t::LZMA:
        return std::make_unique<lzma_compressor>(level);
      default:
        RAISE_EX("Should not be here");
    }
  }

  std::unique_ptr<decompression_stream> decompression_stream::create(compression_type_t type) {
    switch (type) {
      case compression_type_t::ZLIB:
        return std::make_unique<libz_decompressor>(libz_kind_t::ZLIB);
      case compression_type_t::GZIP:
        return std::make_unique<libz_decompressor>(libz_kind_t::GZIP);
      case compression_type_t::ZLIB_RAW:
        return std::make_unique<libz_decompressor>(libz_kind_t::RAW);
      case compression_type_t::ZSTD:
        return std::make_unique<libz_decompressor>(libz_kind_t::ZSTD);
      case compression_type_t::BZ2:
        return std::make_unique<bzip_decompressor>();
      case compression_type_t::LZMA:
        return std::make_unique<lzma_decompressor>();
      default:
        RAISE_EX("Should not be here");
    }
  }
}
