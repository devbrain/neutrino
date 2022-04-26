//
// Created by igor on 08/07/2021.
//

#include <vector>

#include <neutrino/utils/io/deflating_stream.hh>
#include <neutrino/utils/exception.hh>

#include "utils/io/compression_stream/compression_stream.hh"
#include "ios_init.hh"

namespace neutrino::utils::io {
  static constexpr std::size_t STREAM_BUFFER_SIZE = 1024;
  static constexpr std::size_t DEFLATE_BUFFER_SIZE = 32768;

  struct deflating_stream_buf::impl {
    impl (std::istream& istr, compression_type_t type, compression_level_t level)
        : m_istr (&istr),
          m_ostr (nullptr),
          m_buffer (DEFLATE_BUFFER_SIZE),
          m_eof (false),
          m_compressor(compression_stream::create (type, level)) {
    }

    impl (std::ostream& ostr, compression_type_t type, compression_level_t level)
        : m_istr (nullptr),
          m_ostr (&ostr),
          m_buffer (DEFLATE_BUFFER_SIZE),
          m_eof (false),
          m_compressor(compression_stream::create (type, level)) {
    }

    ~impl () {
      try {
        m_compressor->finalize ();
      } catch (...) {}
    }

    std::istream* m_istr;
    std::ostream* m_ostr;
    std::vector<char> m_buffer;
    bool m_eof;
    std::unique_ptr<compression_stream> m_compressor;
  };

  deflating_stream_buf::deflating_stream_buf (std::istream& istr, compression_type_t type, compression_level_t level)
      : buffered_stream_buf (STREAM_BUFFER_SIZE, std::ios::in),
        m_pimpl (spimpl::make_unique_impl<deflating_stream_buf::impl> (istr, type, level)) {

  }

  deflating_stream_buf::deflating_stream_buf (std::ostream& ostr, compression_type_t type, compression_level_t level)
      :
      buffered_stream_buf (STREAM_BUFFER_SIZE, std::ios::out),
      m_pimpl (spimpl::make_unique_impl<deflating_stream_buf::impl> (ostr, type, level)) {

  }

  deflating_stream_buf::~deflating_stream_buf () {
    try {
      close ();
    }
    catch (...) {
    }
  }

  int deflating_stream_buf::close () {
    buffered_stream_buf::sync ();
    m_pimpl->m_istr = nullptr;
    if (m_pimpl->m_ostr) {
      if (m_pimpl->m_compressor->next_out()) {
        auto rc = m_pimpl->m_compressor->compress(compression_stream::FINISH);
        m_pimpl->m_ostr->write (m_pimpl->m_buffer.data (), DEFLATE_BUFFER_SIZE - m_pimpl->m_compressor->avail_out());
        if (!m_pimpl->m_ostr->good ()) {
          RAISE_EX("Failed writing deflated data to output stream");
        }
        m_pimpl->m_compressor->set_next_out((unsigned char*) m_pimpl->m_buffer.data ());
        m_pimpl->m_compressor->set_avail_out(DEFLATE_BUFFER_SIZE);
        while (rc != compression_stream::STREAM_END) {
          rc = m_pimpl->m_compressor->compress(compression_stream::FINISH);
          m_pimpl->m_ostr->write (m_pimpl->m_buffer.data (), DEFLATE_BUFFER_SIZE - m_pimpl->m_compressor->avail_out());
          if (!m_pimpl->m_ostr->good ()) {
            RAISE_EX("Failed writing deflated data to output stream");
          }
          m_pimpl->m_compressor->set_next_out ((unsigned char*) m_pimpl->m_buffer.data ());
          m_pimpl->m_compressor->set_avail_out(DEFLATE_BUFFER_SIZE);
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
      if (m_pimpl->m_compressor->next_out()) {
        auto rc = m_pimpl->m_compressor->compress(compression_stream::SYNC_FLUSH);
        if (rc != compression_stream::OK) {
          RAISE_EX("compression_stream::OK expected");
        }
        m_pimpl->m_ostr->write (m_pimpl->m_buffer.data (), DEFLATE_BUFFER_SIZE - m_pimpl->m_compressor->avail_out());
        if (!m_pimpl->m_ostr->good ()) {
          RAISE_EX("Failed writing deflated data to output stream");
        }
        while (m_pimpl->m_compressor->avail_out() == 0) {
          m_pimpl->m_compressor->set_next_out((unsigned char*) m_pimpl->m_buffer.data ());
          m_pimpl->m_compressor->set_avail_out(DEFLATE_BUFFER_SIZE);
          rc = m_pimpl->m_compressor->compress(compression_stream::SYNC_FLUSH);
          if (rc != compression_stream::OK) {
            RAISE_EX("compression_stream::OK expected");
          }
          m_pimpl->m_ostr->write (m_pimpl->m_buffer.data (), DEFLATE_BUFFER_SIZE - m_pimpl->m_compressor->avail_out());
          if (!m_pimpl->m_ostr->good ()) {
            RAISE_EX("Failed writing deflated data to output stream");
          }
        }
        m_pimpl->m_compressor->set_next_out ((unsigned char*) m_pimpl->m_buffer.data ());
        m_pimpl->m_compressor->set_avail_out(DEFLATE_BUFFER_SIZE);
      }
    }
    return 0;
  }

  int deflating_stream_buf::read_from_device (char* buffer, std::streamsize length) {
    if (!m_pimpl->m_istr) {
      return 0;
    }
    if (m_pimpl->m_compressor->avail_in() == 0 && !m_pimpl->m_eof) {
      int n = 0;
      if (m_pimpl->m_istr->good ()) {
        m_pimpl->m_istr->read (m_pimpl->m_buffer.data (), DEFLATE_BUFFER_SIZE);
        n = static_cast<int>(m_pimpl->m_istr->gcount ());
      }
      if (n > 0) {
        m_pimpl->m_compressor->set_next_in((unsigned char*) m_pimpl->m_buffer.data ());
        m_pimpl->m_compressor->set_avail_in(n);
      }
      else {
        m_pimpl->m_compressor->set_next_in(nullptr);
        m_pimpl->m_compressor->set_avail_in(0);
        m_pimpl->m_eof = true;
      }
    }
    m_pimpl->m_compressor->set_next_out((unsigned char*) buffer);
    m_pimpl->m_compressor->set_avail_out(static_cast<unsigned>(length));
    for (;;) {
      auto rc = m_pimpl->m_compressor->compress(m_pimpl->m_eof ? compression_stream::FINISH : compression_stream::NO_FLUSH);
      if (m_pimpl->m_eof && rc == compression_stream::STREAM_END) {
        m_pimpl->m_istr = nullptr;
        return static_cast<int>(length) - m_pimpl->m_compressor->avail_out();
      }

      if (m_pimpl->m_compressor->avail_out() == 0) {
        return static_cast<int>(length);
      }
      if (m_pimpl->m_compressor->avail_in() == 0) {
        int n = 0;
        if (m_pimpl->m_istr->good ()) {
          m_pimpl->m_istr->read (m_pimpl->m_buffer.data (), DEFLATE_BUFFER_SIZE);
          n = static_cast<int>(m_pimpl->m_istr->gcount ());
        }
        if (n > 0) {
          m_pimpl->m_compressor->set_next_in((unsigned char*) m_pimpl->m_buffer.data ());
          m_pimpl->m_compressor->set_avail_in(n);
        }
        else {
          m_pimpl->m_compressor->set_next_in(nullptr);
          m_pimpl->m_compressor->set_avail_in(0);
          m_pimpl->m_eof = true;
        }
      }
    }
  }

  int deflating_stream_buf::write_to_device (const char* buffer, std::streamsize length) {
    if (length == 0 || !m_pimpl->m_ostr) {
      return 0;
    }

    m_pimpl->m_compressor->set_next_in((unsigned char*) buffer);
    m_pimpl->m_compressor->set_avail_in(static_cast<unsigned>(length));
    m_pimpl->m_compressor->set_next_out((unsigned char*) m_pimpl->m_buffer.data ());
    m_pimpl->m_compressor->set_avail_out(DEFLATE_BUFFER_SIZE);
    for (;;) {
      auto rc = m_pimpl->m_compressor->compress(compression_stream::NO_FLUSH);
      if (rc != compression_stream::OK) {
        RAISE_EX("compression_stream::OK is expected");
      }
      if (m_pimpl->m_compressor->avail_out() == 0) {
        m_pimpl->m_ostr->write (m_pimpl->m_buffer.data (), DEFLATE_BUFFER_SIZE);
        if (!m_pimpl->m_ostr->good ()) {
          RAISE_EX("Failed writing deflated data to output stream");
        }
        m_pimpl->m_compressor->set_next_out((unsigned char*) m_pimpl->m_buffer.data ());
        m_pimpl->m_compressor->set_avail_out(DEFLATE_BUFFER_SIZE);
      }
      if (m_pimpl->m_compressor->avail_in() == 0) {
        m_pimpl->m_ostr->write (m_pimpl->m_buffer.data (), DEFLATE_BUFFER_SIZE - m_pimpl->m_compressor->avail_out());
        if (!m_pimpl->m_ostr->good ()) {
          RAISE_EX("Failed writing deflated data to output stream");
        }
        m_pimpl->m_compressor->set_next_out((unsigned char*) m_pimpl->m_buffer.data ());
        m_pimpl->m_compressor->set_avail_out(DEFLATE_BUFFER_SIZE);
        break;
      }
    }
    return static_cast<int>(length);
  }

  deflating_ios::deflating_ios (std::ostream& ostr, compression_type_t type, compression_level_t level)
      :
      m_buf (ostr, type, level) {
    te_ios_init(&m_buf);
  }



  deflating_ios::deflating_ios (std::istream& istr, compression_type_t type, compression_level_t level)
      :
      m_buf (istr, type, level) {
    te_ios_init(&m_buf);
  }

  deflating_ios::~deflating_ios () = default;

  deflating_stream_buf* deflating_ios::rdbuf () {
    return &m_buf;
  }

  deflating_output_stream::deflating_output_stream (std::ostream& ostr, compression_type_t type, compression_level_t level)
      :
      std::ostream (&m_buf),
      deflating_ios (ostr, type, level) {
  }

  deflating_output_stream::~deflating_output_stream () = default;

  int deflating_output_stream::close () {
    return m_buf.close ();
  }

  int deflating_output_stream::sync () {
    return m_buf.pubsync ();
  }

  deflating_input_stream::deflating_input_stream (std::istream& istr, compression_type_t type, compression_level_t level)
      :
      std::istream (&m_buf),
      deflating_ios (istr, type, level) {
  }

  deflating_input_stream::~deflating_input_stream () = default;
}
