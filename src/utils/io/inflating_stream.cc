//
// Created by igor on 08/07/2021.
//

#include <vector>
#include <neutrino/utils/io/inflating_stream.hh>
#include <neutrino/utils/exception.hh>
#include "utils/io/compression_stream/compression_stream.hh"
#include "ios_init.hh"

namespace neutrino::utils::io {

  static constexpr std::size_t STREAM_BUFFER_SIZE = 1024;
  static constexpr std::size_t INFLATE_BUFFER_SIZE = 32768;

  struct inflating_stream_buf::impl {

    impl (std::istream& istr, compression_type_t type)
        : m_istr (&istr),
          m_ostr (nullptr),
          m_eof (false),
         // m_check ((type != STREAM_ZIP) && (type != STREAM_ZSTD)),
          m_buffer (INFLATE_BUFFER_SIZE),
          m_decompressor(decompression_stream::create (type))
          {
    }

    impl (std::ostream& ostr, compression_type_t type)
        : m_istr (nullptr),
          m_ostr (&ostr),
          m_eof (false),
    //      m_check (type != STREAM_ZIP && type != STREAM_ZSTD),
          m_buffer (INFLATE_BUFFER_SIZE),
          m_decompressor(decompression_stream::create (type))
           {

    }

    ~impl () {
      m_decompressor->finalize();
    }

    std::istream* m_istr;
    std::ostream* m_ostr;


    bool m_eof;
  //  bool m_check;
    std::vector<char> m_buffer;
    std::unique_ptr<decompression_stream> m_decompressor;
  };

  inflating_stream_buf::inflating_stream_buf (std::istream& istr, compression_type_t type)
      :
      buffered_stream_buf (STREAM_BUFFER_SIZE, std::ios::in),
      m_pimpl (spimpl::make_unique_impl<inflating_stream_buf::impl> (istr, type)) {

  }

  inflating_stream_buf::inflating_stream_buf (std::ostream& ostr, compression_type_t type)
      :
      buffered_stream_buf (STREAM_BUFFER_SIZE, std::ios::out),
      m_pimpl (spimpl::make_unique_impl<inflating_stream_buf::impl> (ostr, type)) {
  }


  inflating_stream_buf::~inflating_stream_buf () {
    try {
      close ();
    }
    catch (...) {
    }
    m_pimpl->m_decompressor->finalize();
  }

  int inflating_stream_buf::close () {
    sync ();
    m_pimpl->m_istr = nullptr;
    m_pimpl->m_ostr = nullptr;
    return 0;
  }

  void inflating_stream_buf::reset () {
    m_pimpl->m_decompressor->reset();
    m_pimpl->m_eof = false;
  }

  int inflating_stream_buf::read_from_device (char* buffer, std::streamsize length) {
    if (m_pimpl->m_eof || !m_pimpl->m_istr) {
      return 0;
    }

    if (m_pimpl->m_decompressor->avail_in() == 0) {
      int n = 0;
      if (m_pimpl->m_istr->good ()) {
        m_pimpl->m_istr->read (m_pimpl->m_buffer.data (), INFLATE_BUFFER_SIZE);
        n = static_cast<int>(m_pimpl->m_istr->gcount ());
      }
      m_pimpl->m_decompressor->set_next_in((unsigned char*) m_pimpl->m_buffer.data ());
      m_pimpl->m_decompressor->set_avail_in (n);
    }
    m_pimpl->m_decompressor->set_next_out((unsigned char*) buffer);
    m_pimpl->m_decompressor->set_avail_out(static_cast<unsigned>(length));
    for (;;) {
      auto rc = m_pimpl->m_decompressor->decompress (compression_stream::NO_FLUSH);
      if (rc == compression_stream::DATA_ERROR /* && !m_pimpl->m_check */) {
        if (m_pimpl->m_decompressor->avail_in() == 0) {
          if (m_pimpl->m_istr->good ()) {
            rc = compression_stream::OK;
          }
          else {
            rc = compression_stream::STREAM_END;
          }
        }
      }
      if (rc == compression_stream::STREAM_END) {
        m_pimpl->m_eof = true;
        return static_cast<int>(length) - m_pimpl->m_decompressor->avail_out();
      }
      if (rc != compression_stream::OK) {
        RAISE_EX("Corrupted data");
      }
      if (m_pimpl->m_decompressor->avail_out() == 0) {
        return static_cast<int>(length);
      }
      if (m_pimpl->m_decompressor->avail_in() == 0) {
        int n = 0;
        if (m_pimpl->m_istr->good ()) {
          m_pimpl->m_istr->read (m_pimpl->m_buffer.data (), INFLATE_BUFFER_SIZE);
          n = static_cast<int>(m_pimpl->m_istr->gcount ());
        }
        if (n > 0) {
          m_pimpl->m_decompressor->set_next_in ((unsigned char*) m_pimpl->m_buffer.data ());
          m_pimpl->m_decompressor->set_avail_in(n);
        }
        else {
          return static_cast<int>(length) - m_pimpl->m_decompressor->avail_out();
        }
      }
    }
  }

  int inflating_stream_buf::write_to_device (const char* buffer, std::streamsize length) {
    if (length == 0 || !m_pimpl->m_ostr) {
      return 0;
    }

    m_pimpl->m_decompressor->set_next_in ((unsigned char*) buffer);
    m_pimpl->m_decompressor->set_avail_in(static_cast<unsigned>(length));
    m_pimpl->m_decompressor->set_next_out ((unsigned char*) m_pimpl->m_buffer.data ());
    m_pimpl->m_decompressor->set_avail_out(INFLATE_BUFFER_SIZE);
    for (;;) {
      int rc = m_pimpl->m_decompressor->decompress(compression_stream::NO_FLUSH);
      if (rc == compression_stream::STREAM_END) {
        m_pimpl->m_ostr->write (m_pimpl->m_buffer.data (), INFLATE_BUFFER_SIZE - m_pimpl->m_decompressor->avail_out());
        if (!m_pimpl->m_ostr->good ()) {
          RAISE_EX("Failed writing inflated data to output stream");
        }
        break;
      }

      if (m_pimpl->m_decompressor->avail_out() == 0) {
        m_pimpl->m_ostr->write (m_pimpl->m_buffer.data (), INFLATE_BUFFER_SIZE);
        if (!m_pimpl->m_ostr->good ()) {
          RAISE_EX("Failed writing inflated data to output stream");
        }
        m_pimpl->m_decompressor->set_next_out((unsigned char*) m_pimpl->m_buffer.data ());
        m_pimpl->m_decompressor->set_avail_out(INFLATE_BUFFER_SIZE);
      }
      if (m_pimpl->m_decompressor->avail_in() == 0) {
        m_pimpl->m_ostr->write (m_pimpl->m_buffer.data (), INFLATE_BUFFER_SIZE - m_pimpl->m_decompressor->avail_out());
        if (!m_pimpl->m_ostr->good ()) {
          RAISE_EX("Failed writing inflated data to output stream");
        }
        m_pimpl->m_decompressor->set_next_out((unsigned char*) m_pimpl->m_buffer.data ());
        m_pimpl->m_decompressor->set_avail_out(INFLATE_BUFFER_SIZE);
        break;
      }
    }
    return static_cast<int>(length);
  }

  int inflating_stream_buf::sync () {
    int n = buffered_stream_buf::sync ();
    if (!n && m_pimpl->m_ostr) {
      m_pimpl->m_ostr->flush ();
    }
    return n;
  }

  inflating_ios::inflating_ios (std::ostream& ostr, compression_type_t type)
      :
      _buf (ostr, type) {
    te_ios_init(&_buf);
  }

  inflating_ios::inflating_ios (std::istream& istr, compression_type_t type)
      :
      _buf (istr, type) {
    te_ios_init(&_buf);
  }



  inflating_ios::~inflating_ios () = default;

  inflating_stream_buf* inflating_ios::rdbuf () {
    return &_buf;
  }

  inflating_output_stream::inflating_output_stream (std::ostream& ostr, compression_type_t type)
      :
      std::ostream (&_buf),
      inflating_ios (ostr, type) {
  }



  inflating_output_stream::~inflating_output_stream () = default;

  int inflating_output_stream::close () {
    return _buf.close ();
  }

  inflating_input_stream::inflating_input_stream (std::istream& istr, compression_type_t type)
      :
      std::istream (&_buf),
      inflating_ios (istr, type) {
  }


  inflating_input_stream::~inflating_input_stream () = default;

  void inflating_input_stream::reset () {
    _buf.reset ();
    clear ();
  }
}