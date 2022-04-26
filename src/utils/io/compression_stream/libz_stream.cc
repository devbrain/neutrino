//
// Created by igor on 24/04/2022.
//

#include "libz_stream.hh"
#include "zstd_wrapper.h"
#include <neutrino/utils/exception.hh>
#include <sstream>

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

  namespace detail {
    struct libz_impl : public z_stream {
      libz_impl() : z_stream_s() {
        next_in = nullptr;
        avail_in = 0;
        total_in = 0;
        next_out = nullptr;
        avail_out = 0;
        total_out = 0;
        msg = nullptr;
        state = nullptr;
        zalloc = Z_NULL;
        zfree = Z_NULL;
        opaque = Z_NULL;
        data_type = 0;
        adler = 0;
        reserved = 0;
        is_zstd = false;
      }

      bool is_zstd;
    };
  }

#define THROW(RC) RAISE_EX("zlib error:", error2string (RC), " : ", this->m_pimpl->msg);

  libz_stream::libz_stream ()
  : m_pimpl (spimpl::make_unique_impl<detail::libz_impl>()) {
  }

  libz_stream::~libz_stream () = default;


  const uint8_t* libz_stream::next_in () const {
    return m_pimpl->next_in;
  }

  std::size_t libz_stream::avail_in () const {
    return m_pimpl->avail_in;
  }

  uint8_t* libz_stream::next_out () const {
    return m_pimpl->next_out;

  }

  std::size_t libz_stream::avail_out () const {
    return m_pimpl->avail_out;
  }

  void libz_stream::set_next_in (const unsigned char* in) {
    m_pimpl->next_in = (unsigned char*) in;
  }

  void libz_stream::set_avail_in (std::size_t in) {
    m_pimpl->avail_in = in;
  }

  void libz_stream::set_next_out (const uint8_t* in) {
    m_pimpl->next_out = const_cast<Bytef*>(in);
  }

  void libz_stream::set_avail_out (std::size_t in) {
    m_pimpl->avail_out = in;
  }

  libz_compressor::libz_compressor(libz_kind_t kind, compression_level_t level) {
    int c_level = -1;
    switch (level) {
      case compression_level_t::BEST:
        c_level = 9;
        break;
      case compression_level_t::FAST:
        c_level = 1;
        break;
      case compression_level_t::NORMAL:
        c_level = Z_DEFAULT_COMPRESSION;
    }
    int rc = Z_OK;
    switch (kind) {
      case libz_kind_t::ZSTD:
        m_pimpl->is_zstd = true;
        rc = zstd_deflate_init (m_pimpl.get(), c_level, "", Z_DEFAULT_STRATEGY);
        break;
      case libz_kind_t::GZIP:
        rc = deflateInit2(m_pimpl.get(), c_level, Z_DEFLATED, 15+16, 9, Z_DEFAULT_STRATEGY);
        break;
      case libz_kind_t::RAW:
        rc = deflateInit2(m_pimpl.get(), c_level, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY);
        break;
      case libz_kind_t::ZLIB:
        rc = deflateInit2(m_pimpl.get(), c_level, Z_DEFLATED, 15, 9, Z_DEFAULT_STRATEGY);
        break;
    }
    if (rc != Z_OK) {
      THROW(rc)
    }
  }

  static int convert_flush_mode(compression_stream::flush_mode_t flags) {
    switch (flags) {
      case compression_stream::flush_mode_t::FINISH:
        return Z_FINISH;
      case compression_stream::flush_mode_t::FULL_FLUSH:
        return Z_FULL_FLUSH;
      case compression_stream::flush_mode_t::NO_FLUSH:
        return Z_NO_FLUSH;
      case compression_stream::flush_mode_t::PARTIAL_FLUSH:
        return Z_PARTIAL_FLUSH;
      case compression_stream::flush_mode_t::SYNC_FLUSH:
        return Z_SYNC_FLUSH;
    }
    RAISE_EX("Should not be here");
  }

  libz_compressor::status_t libz_compressor::compress(flush_mode_t flags) {
    int flush = convert_flush_mode (flags);
    int rc = m_pimpl->is_zstd ?
             zstd_deflate (m_pimpl.get(), flush) : deflate (m_pimpl.get(), flush);
    if (rc == Z_OK) {
      return libz_compressor::OK;
    }
    if (rc == Z_STREAM_END) {
      return libz_compressor::STREAM_END;
    }
    THROW(rc);
  }

  void libz_compressor::finalize() {
    if (m_pimpl->is_zstd) {
      zstd_deflate_end (m_pimpl.get());
    } else {
      deflateEnd (m_pimpl.get());
    }
  }

  libz_decompressor::libz_decompressor(libz_kind_t kind) {
    int rc = Z_OK;
    switch (kind) {
      case libz_kind_t::ZSTD:
        m_pimpl->is_zstd = true;
        rc = zstd_inflate_init (m_pimpl.get(), "", 0);
        break;
      case libz_kind_t::GZIP:
        rc = inflateInit2(m_pimpl.get(), 15+16);
        break;
      case libz_kind_t::RAW:
        rc = inflateInit2(m_pimpl.get(), -15);
        break;
      case libz_kind_t::ZLIB:
        rc = inflateInit2(m_pimpl.get(), 15);
        break;
    }
    if (rc != Z_OK) {
      THROW(rc)
    }
  }

  libz_decompressor::status_t libz_decompressor::decompress(flush_mode_t flags) {
    int flush = convert_flush_mode (flags);
    int rc = m_pimpl->is_zstd ?
             zstd_inflate(m_pimpl.get(), flush) : inflate(m_pimpl.get(), flush);
    if (rc == Z_OK) {
      return libz_compressor::OK;
    }
    if (rc == Z_STREAM_END) {
      return libz_compressor::STREAM_END;
    }
    if (rc == Z_DATA_ERROR) {
      return libz_compressor::DATA_ERROR;
    }
    THROW(rc);
  }

  void libz_decompressor::finalize() {
    if (m_pimpl->is_zstd) {
      zstd_inflate_end (m_pimpl.get());
    } else {
      inflateEnd (m_pimpl.get());
    }
  }

  void libz_decompressor::reset() {
    int rc = Z_OK;
    if (m_pimpl->is_zstd) {
      rc = zstd_inflate_reset(m_pimpl.get());
    } else {
      rc = inflateReset (m_pimpl.get());
    }
    if (rc != Z_OK) {
      THROW(rc);
    }
  }
}