//
// Created by igor on 24/04/2022.
//

#include "libz_stream.hh"
#include <neutrino/utils/exception.hh>
#include <sstream>
#include <string>

namespace neutrino::utils::io {

  static std::string error2string (int ret) {
    std::ostringstream os;
    switch (ret) {
      case Z_STREAM_ERROR:
        os << "Z_STREAM_ERROR: ";
        break;
      case Z_DATA_ERROR:
        os << "Z_DATA_ERROR: ";
        break;
      case Z_MEM_ERROR:
        os << "Z_MEM_ERROR: ";
        break;
      case Z_VERSION_ERROR:
        os << "Z_VERSION_ERROR: ";
        break;
      case Z_BUF_ERROR:
        os << "Z_BUF_ERROR: ";
        break;
      default:
        os << "[" << ret << "]: ";
        break;
    }
    return os.str ();
  }

#define THROW RAISE_EX("zlib error:", error2string (m_ret), " : ", this->msg);

  libz_stream::libz_stream (bool is_input, bool is_gzip, int level)
      : m_is_input (is_input) {
    this->zalloc = Z_NULL;
    this->zfree = Z_NULL;
    this->opaque = Z_NULL;
    if (is_input) {
      z_stream::avail_in = 0;
      z_stream::next_in = Z_NULL;
      m_ret = inflateInit2(this, 15 + 32);
    }
    else {
      m_ret = deflateInit2(this, level, Z_DEFLATED, 15 + (is_gzip ? 16 : 0), 8, Z_DEFAULT_STRATEGY);
    }
    if (m_ret != Z_OK) {
      THROW;
    }
  }

  libz_stream::~libz_stream () {
    if (m_is_input) {
      inflateEnd (this);
    }
    else {
      deflateEnd (this);
    }
  }

  int libz_stream::decompress (const int flags) {
    m_ret = inflate (this, flags);
    if (m_ret != Z_OK && m_ret != Z_STREAM_END) {
      THROW;
    }
    return m_ret;
  }

  int libz_stream::compress (const int flags) {
    m_ret = deflate (this, flags);
    if (m_ret != Z_OK && m_ret != Z_STREAM_END && m_ret != Z_BUF_ERROR) {
      THROW;
    }
    return m_ret;
  }

  bool libz_stream::stream_end () const {
    return m_ret == Z_STREAM_END;
  }

  bool libz_stream::done () const {
    return (this->m_ret == Z_BUF_ERROR || this->stream_end ());
  }

  const uint8_t* libz_stream::next_in () const {
    return z_stream::next_in;
  }

  long libz_stream::avail_in () const {
    return z_stream::avail_in;
  }

  uint8_t* libz_stream::next_out () const {
    return z_stream::next_out;

  }

  long libz_stream::avail_out () const {
    return z_stream::avail_out;
  }

  void libz_stream::set_next_in (const unsigned char* in) {
    z_stream::next_in = (unsigned char*) in;
  }

  void libz_stream::set_avail_in (const long in) {
    z_stream::avail_in = in;
  }

  void libz_stream::set_next_out (const uint8_t* in) {
    z_stream::next_out = const_cast<Bytef*>(in);
  }

  void libz_stream::set_avail_out (const long in) {
    z_stream::avail_out = in;
  }
}