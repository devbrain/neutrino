//
// Created by igor on 24/04/2022.
//

#include <thirdparty/bzip2/bzlib.h>
#include <sstream>

#include <neutrino/utils/exception.hh>

#include "bzip_stream.hh"
namespace neutrino::utils::io {

  static std::string error2string (int ret) {
    std::ostringstream os;
    switch (ret) {
      case BZ_CONFIG_ERROR:
        os << "BZ_CONFIG_ERROR";
        break;
      case BZ_SEQUENCE_ERROR:
        os << "BZ_SEQUENCE_ERROR";
        break;
      case BZ_PARAM_ERROR:
        os << "BZ_PARAM_ERROR";
        break;
      case BZ_MEM_ERROR:
        os << "BZ_MEM_ERROR";
        break;
      case BZ_DATA_ERROR:
        os << "BZ_DATA_ERROR";
        break;
      case BZ_DATA_ERROR_MAGIC:
        os << "BZ_DATA_ERROR_MAGIC";
        break;
      case BZ_IO_ERROR:
        os << "BZ_IO_ERROR";
        break;
      case BZ_UNEXPECTED_EOF:
        os << "BZ_UNEXPECTED_EOF";
        break;
      case BZ_OUTBUFF_FULL:
        os << "BZ_OUTBUFF_FULL";
        break;
      default:
        os << "[" << ret << "]: ";
        break;
    }
    return os.str ();
  }

#define THROW RAISE_EX("bzip2 error: ", error2string(ret))

  namespace detail {
    struct bzip_impl : public bz_stream  {
      bzip_impl() {
        bzalloc = nullptr;
        bzfree = nullptr;
        opaque = nullptr;
        next_in = nullptr;
        avail_in = 0;
        next_out = nullptr;
        avail_out = 0;
      }
    };
  }

  bzip_stream::bzip_stream ()
      : m_pimpl(spimpl::make_unique_impl<detail::bzip_impl>()) {
  }

  bzip_stream::~bzip_stream () = default;

  const uint8_t* bzip_stream::next_in () const {
    return (uint8_t*) m_pimpl->next_in;
  }

  std::size_t bzip_stream::avail_in () const {
    return m_pimpl->avail_in;
  }

  uint8_t* bzip_stream::next_out () const {
    return (uint8_t*) m_pimpl->next_out;
  }

  std::size_t bzip_stream::avail_out () const {
    return m_pimpl->avail_out;
  }

  void bzip_stream::set_next_in (const unsigned char* in) {
    m_pimpl->next_in = (char*) in;
  }

  void bzip_stream::set_avail_in (std::size_t in) {
    m_pimpl->avail_in = in;
  }

  void bzip_stream::set_next_out (const uint8_t* in) {
    m_pimpl->next_out = (char*) in;
  }

  void bzip_stream::set_avail_out (std::size_t in) {
    m_pimpl->avail_out = in;
  }

  bzip_compressor::bzip_compressor(compression_level_t level) {
    int c_level = -1;
    switch (level) {
      case compression_level_t::BEST:
        c_level = 9;
        break;
      case compression_level_t::FAST:
        c_level = 1;
        break;
      case compression_level_t::NORMAL:
        c_level = 5;
    }

    auto ret = BZ2_bzCompressInit (this->m_pimpl.get(), c_level, 0, 0);
    if (ret != BZ_OK) {
      THROW;
    }
  }

  static int convert_flush_mode(compression_stream::flush_mode_t flags) {
    switch (flags) {
      case compression_stream::flush_mode_t::FINISH:
        return BZ_FINISH;
      case compression_stream::flush_mode_t::FULL_FLUSH:
        return BZ_FLUSH;
      case compression_stream::flush_mode_t::NO_FLUSH:
        return BZ_RUN;
      case compression_stream::flush_mode_t::PARTIAL_FLUSH:
        return BZ_FLUSH;
      case compression_stream::flush_mode_t::SYNC_FLUSH:
        return BZ_FLUSH;
    }
    RAISE_EX("Should not be here");
  }

  bzip_compressor::status_t bzip_compressor::compress(flush_mode_t flags) {
    int flush_mode = convert_flush_mode (flags);
    auto ret = BZ2_bzCompress (this->m_pimpl.get(), flush_mode);
    switch (ret) {
      case BZ_OK:
      case BZ_RUN_OK:
      case BZ_FLUSH_OK:
      case BZ_FINISH_OK:
        return bzip_compressor::OK;
      case BZ_STREAM_END:
        return bzip_compressor::STREAM_END;
    }
    THROW;
  }

  void bzip_compressor::finalize() {
    BZ2_bzCompressEnd (m_pimpl.get());
  }

  bzip_decompressor::bzip_decompressor() {
    auto ret = BZ2_bzDecompressInit (m_pimpl.get(), 0, 0);
    if (ret != BZ_OK) {
      THROW;
    }
  }

  bzip_decompressor::status_t bzip_decompressor::decompress([[maybe_unused]] flush_mode_t flags) {
    auto ret = BZ2_bzDecompress (m_pimpl.get());
    switch (ret) {
      case BZ_OK:
        return bzip_compressor::OK;
      case BZ_STREAM_END:
        return bzip_compressor::STREAM_END;
      case BZ_DATA_ERROR:
        return bzip_compressor::DATA_ERROR;
    }
    THROW;
  }

  void bzip_decompressor::finalize() {
    BZ2_bzDecompressEnd (m_pimpl.get());
  }

  void bzip_decompressor::reset() {

  }
}