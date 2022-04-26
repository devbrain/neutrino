//
// Created by igor on 08/07/2021.
//

#ifndef NEUTRINO_UTILS_IO_INFLATING_STREAM_HH
#define NEUTRINO_UTILS_IO_INFLATING_STREAM_HH

#include <ostream>
#include <istream>
#include <neutrino/utils/io/buffered_stream_buf.hh>
#include <neutrino/utils/io/compression_type.hh>
#include <neutrino/utils/spimpl.h>

namespace neutrino::utils::io {
  /// This is the streambuf class used by InflatingInputStream and InflatingOutputStream.
  /// The actual work is delegated to zlib (see http://zlib.net).
  /// Both zlib (deflate) streams and gzip streams are supported.
  /// Output streams should always call close() to ensure
  /// proper completion of decompression.
  class inflating_stream_buf : public buffered_stream_buf {
    public:
      /// Creates an InflatingStreamBuf for expanding the compressed data read from
      /// the give input stream.
      inflating_stream_buf (std::istream& istr, compression_type_t type);

      /// Creates an InflatingStreamBuf for expanding the compressed data passed through
      /// and forwarding it to the given output stream.
      inflating_stream_buf (std::ostream& ostr, compression_type_t type);

      ~inflating_stream_buf () override;

      /// Finishes up the stream.
      ///
      /// Must be called when inflating to an output stream.
      int close ();

      /// Resets the stream buffer.
      void reset ();
    protected:
      int read_from_device (char* buffer, std::streamsize length) override;
      int write_to_device (const char* buffer, std::streamsize length) override;
      int sync () override;
    private:
      struct impl;
      spimpl::unique_impl_ptr<impl> m_pimpl;

  };

  /// The base class for InflatingOutputStream and InflatingInputStream.
  ///
  /// This class is needed to ensure the correct initialization
  /// order of the stream buffer and base classes.
  class inflating_ios : public virtual std::ios {
    public:
      /// Creates an InflatingIOS for expanding the compressed data passed through
      /// and forwarding it to the given output stream.
      explicit inflating_ios (std::ostream& ostr,
                              compression_type_t type = compression_type_t::ZLIB);


      /// Creates an InflatingIOS for expanding the compressed data read from
      /// the given input stream.
      explicit inflating_ios (std::istream& istr,
                              compression_type_t type = compression_type_t::ZLIB);

      /// Destroys the InflatingIOS.
      ~inflating_ios () override;

      /// Returns a pointer to the underlying stream buffer.
      inflating_stream_buf* rdbuf ();

    protected:
      inflating_stream_buf _buf;
  };

  /// This stream decompresses all data passing through it
  /// using zlib's inflate algorithm.
  ///
  /// After all data has been written to the stream, close()
  /// must be called to ensure completion of decompression.
  class inflating_output_stream : public std::ostream, public inflating_ios {
    public:
      /// Creates an InflatingOutputStream for expanding the compressed data passed through
      /// and forwarding it to the given output stream.
      explicit inflating_output_stream (std::ostream& ostr,
                                        compression_type_t type = compression_type_t::ZLIB);



      ~inflating_output_stream () override;

      /// Finishes up the stream.
      ///
      /// Must be called to ensure all data is properly written to
      /// the target output stream.
      int close ();
  };

  /// This stream decompresses all data passing through it
  /// using zlib's inflate algorithm.
  /// Example:
  ///     std::ifstream istr("data.gz", std::ios::binary);
  ///     InflatingInputStream inflater(istr, InflatingStreamBuf::STREAM_GZIP);
  ///     std::string data;
  ///     inflater >> data;
  ///
  /// The underlying input stream can contain more than one gzip/deflate stream.
  /// After a gzip/deflate stream has been processed, reset() can be called
  /// to inflate the next stream.
  class inflating_input_stream : public std::istream, public inflating_ios {
    public:
      /// Creates an InflatingInputStream for expanding the compressed data read from
      /// the given input stream.
      explicit inflating_input_stream (std::istream& istr,
                                       compression_type_t type = compression_type_t::ZLIB);


      ~inflating_input_stream () override;

      /// Resets the zlib machinery so that another zlib stream can be read from
      /// the same underlying input stream.
      void reset ();

  };
}

#endif
