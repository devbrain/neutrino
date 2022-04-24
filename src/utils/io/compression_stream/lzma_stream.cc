//
// Created by igor on 24/04/2022.
//

#include "lzma_stream.hh"
#include <neutrino/utils/exception.hh>

namespace neutrino::utils::io {

  static std::string error2string (int ret) {
    std::ostringstream os;
    switch (ret) {
      case LZMA_MEM_ERROR:
        os << "LZMA_MEM_ERROR";
        break;
      case LZMA_OPTIONS_ERROR:
        os << "LZMA_OPTIONS_ERROR";
        break;
      case LZMA_UNSUPPORTED_CHECK:
        os << "LZMA_UNSUPPORTED_CHECK";
        break;
      case LZMA_PROG_ERROR:
        os << "LZMA_PROG_ERROR";
        break;
      case LZMA_BUF_ERROR:
        os << "LZMA_BUF_ERROR";
        break;
      case LZMA_DATA_ERROR:
        os << "LZMA_DATA_ERROR";
        break;
      case LZMA_FORMAT_ERROR:
        os << "LZMA_FORMAT_ERROR";
        break;
      case LZMA_NO_CHECK:
        os << "LZMA_NO_CHECK";
        break;
      case LZMA_MEMLIMIT_ERROR:
        os << "LZMA_MEMLIMIT_ERROR";
        break;
      default:
        os << "[" << ret << "]: ";
        break;
    }
    return os.str ();
  }

#define THROW RAISE_EX("lzma error: ", error2string(ret))

  lzma_compression_stream::lzma_compression_stream (bool isinput, int level, int flags)
      : lzma_stream (LZMA_STREAM_INIT), is_input (isinput) {
    if (is_input) {
      lzma_stream::avail_in = 0;
      lzma_stream::next_in = nullptr;
      ret = lzma_auto_decoder (this, UINT64_MAX, flags);
    }
    else {
      ret = lzma_easy_encoder (this, level, LZMA_CHECK_CRC64);
    }
    if (ret != LZMA_OK) {
      THROW;
    }

  }

  lzma_compression_stream::~lzma_compression_stream () {
    lzma_end (this);
  }

  int lzma_compression_stream::decompress (const int flags) {
    ret = lzma_code (this, LZMA_RUN);
    if (ret != LZMA_OK && ret != LZMA_STREAM_END) {
      THROW;
    }
    return (int) ret;
  }

  int lzma_compression_stream::compress (const int flags) {
    ret = lzma_code (this, (lzma_action) flags);
    if (ret != LZMA_OK && ret != LZMA_STREAM_END && ret != LZMA_BUF_ERROR) {
      THROW;
    }
    return (int) ret;
  }

  bool lzma_compression_stream::stream_end () const {
    return this->ret == LZMA_STREAM_END;
  }

  bool lzma_compression_stream::done () const {
    return (this->ret == LZMA_BUF_ERROR || this->stream_end ());
  }

  const uint8_t* lzma_compression_stream::next_in () const {
    return lzma_stream::next_in;
  }

  long lzma_compression_stream::avail_in () const {
    return lzma_stream::avail_in;
  }

  uint8_t* lzma_compression_stream::next_out () const {
    return lzma_stream::next_out;
  }

  long lzma_compression_stream::avail_out () const {
    return lzma_stream::avail_out;
  }

  void lzma_compression_stream::set_next_in (const unsigned char* in) {
    lzma_stream::next_in = in;
  }

  void lzma_compression_stream::set_avail_in (const long in) {
    lzma_stream::avail_in = in;
  }

  void lzma_compression_stream::set_next_out (const uint8_t* in) {
    lzma_stream::next_out = const_cast<uint8_t*>(in);
  }

  void lzma_compression_stream::set_avail_out (const long in) {
    lzma_stream::avail_out = in;
  }
}