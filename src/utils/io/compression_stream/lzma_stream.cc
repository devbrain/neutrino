//
// Created by igor on 24/04/2022.
//

#include "thirdparty/liblzma/src/liblzma/api/lzma.h"
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

  namespace detail {
    struct lzma_impl : public lzma_stream {
      lzma_impl()
      : lzma_stream (LZMA_STREAM_INIT) {
      }
    };
  }

  liblzma_stream::liblzma_stream ()
      : m_pimpl(spimpl::make_unique_impl<detail::lzma_impl>()) {
  }

  liblzma_stream::~liblzma_stream () = default;


  const uint8_t* liblzma_stream::next_in () const {
    return m_pimpl->next_in;
  }

  std::size_t liblzma_stream::avail_in () const {
    return m_pimpl->avail_in;
  }

  uint8_t* liblzma_stream::next_out () const {
    return m_pimpl->next_out;
  }

  std::size_t liblzma_stream::avail_out () const {
    return m_pimpl->avail_out;
  }

  void liblzma_stream::set_next_in (const unsigned char* in) {
    m_pimpl->next_in = in;
  }

  void liblzma_stream::set_avail_in (std::size_t in) {
    m_pimpl->avail_in = in;
  }

  void liblzma_stream::set_next_out (const uint8_t* in) {
    m_pimpl->next_out = const_cast<uint8_t*>(in);
  }

  void liblzma_stream::set_avail_out (std::size_t in) {
    m_pimpl->avail_out = in;
  }

  lzma_compressor::lzma_compressor(compression_level_t level) {
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
    auto ret = lzma_easy_encoder (m_pimpl.get(), c_level, LZMA_CHECK_CRC64);
    if (ret != LZMA_OK) {
      THROW;
    }
  }

  static lzma_action convert_lzma_flags(compression_stream::flush_mode_t flags) {
    switch (flags) {
      case compression_stream::flush_mode_t::FINISH:
        return LZMA_FINISH;
      case compression_stream::flush_mode_t::FULL_FLUSH:
        return LZMA_FULL_FLUSH;
      case compression_stream::flush_mode_t::NO_FLUSH:
        return LZMA_RUN;
      case compression_stream::flush_mode_t::PARTIAL_FLUSH:
        return LZMA_SYNC_FLUSH;
      case compression_stream::flush_mode_t::SYNC_FLUSH:
        return LZMA_SYNC_FLUSH;
    }
    RAISE_EX("Should not be here");
  }

  lzma_compressor::status_t lzma_compressor::compress(flush_mode_t flags) {
    auto action = convert_lzma_flags (flags);
    auto ret = lzma_code (m_pimpl.get(), action);
    switch (ret) {
      case LZMA_OK:
        return compression_stream::OK;
      case LZMA_STREAM_END:
      case LZMA_BUF_ERROR:
        return compression_stream::STREAM_END;
      default:
        THROW;
    }
    THROW;
  }

  void lzma_compressor::finalize() {
      lzma_end (m_pimpl.get());
  }

  lzma_decompressor::lzma_decompressor() {
    auto ret = lzma_auto_decoder (m_pimpl.get(), UINT64_MAX, 0);
    if (ret != LZMA_OK) {
      THROW;
    }
  }



  lzma_decompressor::status_t lzma_decompressor::decompress(flush_mode_t flags) {
    auto action = convert_lzma_flags (flags);
    auto ret = lzma_code (m_pimpl.get(), action);
    switch (ret) {
      case LZMA_OK:
        return compression_stream::OK;
      case LZMA_STREAM_END:
      case LZMA_BUF_ERROR:
        return compression_stream::STREAM_END;
      default:
        THROW;
    }
    THROW;
  }

  void lzma_decompressor::finalize() {
    lzma_end (m_pimpl.get());
  }

  void lzma_decompressor::reset() {

  }
}