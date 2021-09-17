//
// Created by igor on 08/07/2021.
//

#include <vector>

#include <neutrino/utils/io/deflating_stream.hh>
#include <neutrino/utils/exception.hh>
#include <thirdparty/zlib/zlib.h>
#include "ios_init.hh"
#include "utils/io/compression_stream/zstd_wrapper.h"

namespace neutrino::utils::io {
  static constexpr auto STREAM_BUFFER_SIZE = 1024;
  static constexpr auto DEFLATE_BUFFER_SIZE = 32768;

  struct deflating_stream_buf::impl {
    impl (std::istream& istr, deflating_stream_buf::type_t type, int level)
        : m_istr (&istr),
          m_ostr (nullptr),
          m_buffer (DEFLATE_BUFFER_SIZE),
          m_type (type),
          m_eof (false) {
      m_zstr.next_in = nullptr;
      m_zstr.avail_in = 0;
      m_zstr.total_in = 0;
      m_zstr.next_out = nullptr;
      m_zstr.avail_out = 0;
      m_zstr.total_out = 0;
      m_zstr.msg = nullptr;
      m_zstr.state = nullptr;
      m_zstr.zalloc = Z_NULL;
      m_zstr.zfree = Z_NULL;
      m_zstr.opaque = Z_NULL;
      m_zstr.data_type = 0;
      m_zstr.adler = 0;
      m_zstr.reserved = 0;

      int rc = (m_type == STREAM_ZSTD) ?
               zstd_deflate_init (&m_zstr, level, "", Z_DEFAULT_STRATEGY) :
               deflateInit2(&m_zstr, level, Z_DEFLATED, 15 + (type == STREAM_GZIP ? 16 : 0), 8,
                            Z_DEFAULT_STRATEGY);
      if (rc != Z_OK) {
        RAISE_EX(zError (rc));
      }
    }

    impl (std::istream& istr, int windowBits, int level)
        : m_istr (&istr),
          m_ostr (nullptr),
          m_buffer (DEFLATE_BUFFER_SIZE),
          m_type (STREAM_GZIP),
          m_eof (false) {
      m_zstr.zalloc = Z_NULL;
      m_zstr.zfree = Z_NULL;
      m_zstr.opaque = Z_NULL;
      m_zstr.next_in = nullptr;
      m_zstr.avail_in = 0;
      m_zstr.next_out = nullptr;
      m_zstr.avail_out = 0;

      int rc = (m_type == STREAM_ZSTD) ?
               zstd_deflate_init (&m_zstr, level, "", Z_DEFAULT_STRATEGY) :
               deflateInit2(&m_zstr, level, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY);

      if (rc != Z_OK) {
        RAISE_EX(zError (rc));
      }
    }

    impl (std::ostream& ostr, deflating_stream_buf::type_t type, int level)
        : m_istr (nullptr),
          m_ostr (&ostr),
          m_buffer (DEFLATE_BUFFER_SIZE),
          m_type (type),
          m_eof (false) {
      m_zstr.zalloc = Z_NULL;
      m_zstr.zfree = Z_NULL;
      m_zstr.opaque = Z_NULL;
      m_zstr.next_in = nullptr;
      m_zstr.avail_in = 0;
      m_zstr.next_out = nullptr;
      m_zstr.avail_out = 0;

      int rc = (m_type == STREAM_ZSTD) ?
               zstd_deflate_init (&m_zstr, level, "", Z_DEFAULT_STRATEGY) :
               deflateInit2(&m_zstr, level, Z_DEFLATED, 15 + (type == STREAM_GZIP ? 16 : 0), 8,
                            Z_DEFAULT_STRATEGY);
      if (rc != Z_OK) {
        RAISE_EX(zError (rc));
      }
    }

    impl (std::ostream& ostr, int windowBits, int level)
        : m_istr (nullptr),
          m_ostr (&ostr),
          m_buffer (DEFLATE_BUFFER_SIZE),
          m_type (STREAM_GZIP),
          m_eof (false) {
      m_zstr.zalloc = Z_NULL;
      m_zstr.zfree = Z_NULL;
      m_zstr.opaque = Z_NULL;
      m_zstr.next_in = nullptr;
      m_zstr.avail_in = 0;
      m_zstr.next_out = nullptr;
      m_zstr.avail_out = 0;

      int rc = (m_type == STREAM_ZSTD) ?
               zstd_deflate_init (&m_zstr, level, "", Z_DEFAULT_STRATEGY) :
               deflateInit2(&m_zstr, level, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY);
      if (rc != Z_OK) {
        RAISE_EX(zError (rc));
      }
    }

    int do_deflate (z_streamp strm, int flush) {
      return (m_type == STREAM_ZSTD) ? zstd_deflate (strm, flush) : deflate (strm, flush);
    }

    ~impl () {
      if (m_type == STREAM_ZSTD) {
        zstd_deflate_end (&m_zstr);
      }
    }

    std::istream* m_istr;
    std::ostream* m_ostr;
    std::vector<char> m_buffer;
    z_stream m_zstr;
    deflating_stream_buf::type_t m_type;
    bool m_eof;
  };

  deflating_stream_buf::deflating_stream_buf (std::istream& istr, type_t type, zlib_compression_t level)
      : buffered_stream_buf (STREAM_BUFFER_SIZE, std::ios::in),
        m_pimpl (spimpl::make_unique_impl<deflating_stream_buf::impl> (istr, type, level)) {

  }

  deflating_stream_buf::deflating_stream_buf (std::istream& istr, int windowBits, zlib_compression_t level)
      : buffered_stream_buf (STREAM_BUFFER_SIZE, std::ios::in),
        m_pimpl (spimpl::make_unique_impl<deflating_stream_buf::impl> (istr, windowBits, level)) {

  }

  deflating_stream_buf::deflating_stream_buf (std::ostream& ostr, type_t type, zlib_compression_t level)
      :
      buffered_stream_buf (STREAM_BUFFER_SIZE, std::ios::out),
      m_pimpl (spimpl::make_unique_impl<deflating_stream_buf::impl> (ostr, type, level)) {

  }

  deflating_stream_buf::deflating_stream_buf (std::ostream& ostr, int windowBits, zlib_compression_t level)
      : buffered_stream_buf (STREAM_BUFFER_SIZE, std::ios::out),
        m_pimpl (spimpl::make_unique_impl<deflating_stream_buf::impl> (ostr, windowBits, level)) {

  }

  deflating_stream_buf::~deflating_stream_buf () {
    try {
      close ();
    }
    catch (...) {
    }
    deflateEnd (&m_pimpl->m_zstr);
  }

  int deflating_stream_buf::close () {
    buffered_stream_buf::sync ();
    m_pimpl->m_istr = 0;
    if (m_pimpl->m_ostr) {
      if (m_pimpl->m_zstr.next_out) {
        int rc = m_pimpl->do_deflate (&m_pimpl->m_zstr, Z_FINISH);
        if (rc != Z_OK && rc != Z_STREAM_END) {
          RAISE_EX(zError (rc));
        }
        m_pimpl->m_ostr->write (m_pimpl->m_buffer.data (), DEFLATE_BUFFER_SIZE - m_pimpl->m_zstr.avail_out);
        if (!m_pimpl->m_ostr->good ()) {
          RAISE_EX("Failed writing deflated data to output stream");
        }
        m_pimpl->m_zstr.next_out = (unsigned char*) m_pimpl->m_buffer.data ();
        m_pimpl->m_zstr.avail_out = DEFLATE_BUFFER_SIZE;
        while (rc != Z_STREAM_END) {
          rc = m_pimpl->do_deflate (&m_pimpl->m_zstr, Z_FINISH);
          if (rc != Z_OK && rc != Z_STREAM_END) {
            RAISE_EX(zError (rc));
          }
          m_pimpl->m_ostr->write (m_pimpl->m_buffer.data (), DEFLATE_BUFFER_SIZE - m_pimpl->m_zstr.avail_out);
          if (!m_pimpl->m_ostr->good ()) {
            RAISE_EX("Failed writing deflated data to output stream");
          }
          m_pimpl->m_zstr.next_out = (unsigned char*) m_pimpl->m_buffer.data ();
          m_pimpl->m_zstr.avail_out = DEFLATE_BUFFER_SIZE;
        }
      }
      m_pimpl->m_ostr->flush ();
      m_pimpl->m_ostr = nullptr;
    }
    return 0;
  }

  int deflating_stream_buf::sync () {
    if (buffered_stream_buf::sync ()) {
      return -1;
    }

    if (m_pimpl->m_ostr) {
      if (m_pimpl->m_zstr.next_out) {
        int rc = m_pimpl->do_deflate (&m_pimpl->m_zstr, Z_SYNC_FLUSH);
        if (rc != Z_OK) {
          RAISE_EX(zError (rc));
        }
        m_pimpl->m_ostr->write (m_pimpl->m_buffer.data (), DEFLATE_BUFFER_SIZE - m_pimpl->m_zstr.avail_out);
        if (!m_pimpl->m_ostr->good ()) {
          RAISE_EX("Failed writing deflated data to output stream");
        }
        while (m_pimpl->m_zstr.avail_out == 0) {
          m_pimpl->m_zstr.next_out = (unsigned char*) m_pimpl->m_buffer.data ();
          m_pimpl->m_zstr.avail_out = DEFLATE_BUFFER_SIZE;
          rc = m_pimpl->do_deflate (&m_pimpl->m_zstr, Z_SYNC_FLUSH);
          if (rc != Z_OK) {
            RAISE_EX(zError (rc));
          }
          m_pimpl->m_ostr->write (m_pimpl->m_buffer.data (), DEFLATE_BUFFER_SIZE - m_pimpl->m_zstr.avail_out);
          if (!m_pimpl->m_ostr->good ()) {
            RAISE_EX("Failed writing deflated data to output stream");
          }
        }
        m_pimpl->m_zstr.next_out = (unsigned char*) m_pimpl->m_buffer.data ();
        m_pimpl->m_zstr.avail_out = DEFLATE_BUFFER_SIZE;
      }
      // NOTE: This breaks the Zip library and causes corruption in some files.
      // See GH #1828
      // _pOstr->flush();
    }
    return 0;
  }

  int deflating_stream_buf::read_from_device (char* buffer, std::streamsize length) {
    if (!m_pimpl->m_istr) {
      return 0;
    }
    if (m_pimpl->m_zstr.avail_in == 0 && !m_pimpl->m_eof) {
      int n = 0;
      if (m_pimpl->m_istr->good ()) {
        m_pimpl->m_istr->read (m_pimpl->m_buffer.data (), DEFLATE_BUFFER_SIZE);
        n = static_cast<int>(m_pimpl->m_istr->gcount ());
      }
      if (n > 0) {
        m_pimpl->m_zstr.next_in = (unsigned char*) m_pimpl->m_buffer.data ();
        m_pimpl->m_zstr.avail_in = n;
      }
      else {
        m_pimpl->m_zstr.next_in = nullptr;
        m_pimpl->m_zstr.avail_in = 0;
        m_pimpl->m_eof = true;
      }
    }
    m_pimpl->m_zstr.next_out = (unsigned char*) buffer;
    m_pimpl->m_zstr.avail_out = static_cast<unsigned>(length);
    for (;;) {
      int rc = m_pimpl->do_deflate (&m_pimpl->m_zstr, m_pimpl->m_eof ? Z_FINISH : Z_NO_FLUSH);
      if (m_pimpl->m_eof && rc == Z_STREAM_END) {
        m_pimpl->m_istr = nullptr;
        return static_cast<int>(length) - m_pimpl->m_zstr.avail_out;
      }
      if (rc != Z_OK) {
        RAISE_EX(zError (rc));
      }
      if (m_pimpl->m_zstr.avail_out == 0) {
        return static_cast<int>(length);
      }
      if (m_pimpl->m_zstr.avail_in == 0) {
        int n = 0;
        if (m_pimpl->m_istr->good ()) {
          m_pimpl->m_istr->read (m_pimpl->m_buffer.data (), DEFLATE_BUFFER_SIZE);
          n = static_cast<int>(m_pimpl->m_istr->gcount ());
        }
        if (n > 0) {
          m_pimpl->m_zstr.next_in = (unsigned char*) m_pimpl->m_buffer.data ();
          m_pimpl->m_zstr.avail_in = n;
        }
        else {
          m_pimpl->m_zstr.next_in = nullptr;
          m_pimpl->m_zstr.avail_in = 0;
          m_pimpl->m_eof = true;
        }
      }
    }
  }

  int deflating_stream_buf::write_to_device (const char* buffer, std::streamsize length) {
    if (length == 0 || !m_pimpl->m_ostr) {
      return 0;
    }

    m_pimpl->m_zstr.next_in = (unsigned char*) buffer;
    m_pimpl->m_zstr.avail_in = static_cast<unsigned>(length);
    m_pimpl->m_zstr.next_out = (unsigned char*) m_pimpl->m_buffer.data ();
    m_pimpl->m_zstr.avail_out = DEFLATE_BUFFER_SIZE;
    for (;;) {
      int rc = m_pimpl->do_deflate (&m_pimpl->m_zstr, Z_NO_FLUSH);
      if (rc != Z_OK) {
        RAISE_EX(zError (rc));
      }
      if (m_pimpl->m_zstr.avail_out == 0) {
        m_pimpl->m_ostr->write (m_pimpl->m_buffer.data (), DEFLATE_BUFFER_SIZE);
        if (!m_pimpl->m_ostr->good ()) {
          RAISE_EX("Failed writing deflated data to output stream");
        }
        m_pimpl->m_zstr.next_out = (unsigned char*) m_pimpl->m_buffer.data ();
        m_pimpl->m_zstr.avail_out = DEFLATE_BUFFER_SIZE;
      }
      if (m_pimpl->m_zstr.avail_in == 0) {
        m_pimpl->m_ostr->write (m_pimpl->m_buffer.data (), DEFLATE_BUFFER_SIZE - m_pimpl->m_zstr.avail_out);
        if (!m_pimpl->m_ostr->good ()) {
          RAISE_EX("Failed writing deflated data to output stream");
        }
        m_pimpl->m_zstr.next_out = (unsigned char*) m_pimpl->m_buffer.data ();
        m_pimpl->m_zstr.avail_out = DEFLATE_BUFFER_SIZE;
        break;
      }
    }
    return static_cast<int>(length);
  }

  deflating_ios::deflating_ios (std::ostream& ostr, deflating_stream_buf::type_t type, zlib_compression_t level)
      :
      m_buf (ostr, type, level) {
    te_ios_init(&m_buf);
  }

  deflating_ios::deflating_ios (std::ostream& ostr, int windowBits, zlib_compression_t level)
      :
      m_buf (ostr, windowBits, level) {
    te_ios_init(&m_buf);
  }

  deflating_ios::deflating_ios (std::istream& istr, deflating_stream_buf::type_t type, zlib_compression_t level)
      :
      m_buf (istr, type, level) {
    te_ios_init(&m_buf);
  }

  deflating_ios::deflating_ios (std::istream& istr, int windowBits, zlib_compression_t level)
      :
      m_buf (istr, windowBits, level) {
    te_ios_init(&m_buf);
  }

  deflating_ios::~deflating_ios () = default;

  deflating_stream_buf* deflating_ios::rdbuf () {
    return &m_buf;
  }

  deflating_output_stream::deflating_output_stream (std::ostream& ostr, deflating_stream_buf::type_t type,
                                                    zlib_compression_t level)
      :
      std::ostream (&m_buf),
      deflating_ios (ostr, type, level) {
  }

  deflating_output_stream::deflating_output_stream (std::ostream& ostr, int windowBits, zlib_compression_t level)
      :
      std::ostream (&m_buf),
      deflating_ios (ostr, windowBits, level) {
  }

  deflating_output_stream::~deflating_output_stream () {
  }

  int deflating_output_stream::close () {
    return m_buf.close ();
  }

  int deflating_output_stream::sync () {
    return m_buf.pubsync ();
  }

  deflating_input_stream::deflating_input_stream (std::istream& istr, deflating_stream_buf::type_t type,
                                                  zlib_compression_t level)
      :
      std::istream (&m_buf),
      deflating_ios (istr, type, level) {
  }

  deflating_input_stream::deflating_input_stream (std::istream& istr, int windowBits, zlib_compression_t level)
      :
      std::istream (&m_buf),
      deflating_ios (istr, windowBits, level) {
  }

  deflating_input_stream::~deflating_input_stream () {
  }

}