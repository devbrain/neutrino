//
// Created by igor on 24/04/2022.
//

#include "bzip_stream.hh"
#include <sstream>
#include <string>
#include <neutrino/utils/exception.hh>

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

  bzip_stream::bzip_stream (bool is_input, int level, int wf)
      : m_is_input (is_input) {
    this->bzalloc = nullptr;
    this->bzfree = nullptr;
    this->opaque = nullptr;
    if (is_input) {
      bz_stream::avail_in = 0;
      bz_stream::next_in = nullptr;
      ret = BZ2_bzDecompressInit (this, 0, 0);
    }
    else {
      ret = BZ2_bzCompressInit (this, level, 0, wf);
    }
    if (ret != BZ_OK) {
      THROW;
    }
  }

  bzip_stream::~bzip_stream () {
    if (m_is_input) {
      BZ2_bzDecompressEnd (this);
    }
    else {
      BZ2_bzCompressEnd (this);
    }
  }

  int bzip_stream::decompress ([[maybe_unused]] flush_mode_t flags) {
    ret = BZ2_bzDecompress (this);
    if (ret != BZ_OK && ret != BZ_STREAM_END) {
      THROW;
    }
    return ret;
  }

  int bzip_stream::compress ([[maybe_unused]] flush_mode_t flags) {
    ret = BZ2_bzCompress (this, flags);
    if (!ret) {
      THROW;
    }
    return ret;
  }

  bool bzip_stream::stream_end () const {
    return this->ret == BZ_STREAM_END;
  }

  bool bzip_stream::done () const {
    return this->stream_end ();
  }

  const uint8_t* bzip_stream::next_in () const {
    return (uint8_t*) bz_stream::next_in;
  }

  long bzip_stream::avail_in () const {
    return bz_stream::avail_in;
  }

  uint8_t* bzip_stream::next_out () const {
    return (uint8_t*) bz_stream::next_out;
  }

  long bzip_stream::avail_out () const {
    return bz_stream::avail_out;
  }

  void bzip_stream::set_next_in (const unsigned char* in) {
    bz_stream::next_in = (char*) in;
  }

  void bzip_stream::set_avail_in (const long in) {
    bz_stream::avail_in = in;
  }

  void bzip_stream::set_next_out (const uint8_t* in) {
    bz_stream::next_out = (char*) in;
  }

  void bzip_stream::set_avail_out (const long in) {
    bz_stream::avail_out = in;
  }
}