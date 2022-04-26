//
// Created by igor on 08/07/2021.
//

#ifndef NEUTRINO_UTILS_IO_ZLIB_OSTREAM_HH
#define NEUTRINO_UTILS_IO_ZLIB_OSTREAM_HH

#include <iosfwd>
#include <ostream>
#include <istream>
#include <neutrino/utils/io/buffered_stream_buf.hh>
#include <neutrino/utils/io/compression_type.hh>
#include <neutrino/utils/spimpl.h>

namespace neutrino::utils::io {

  /**
   This is the streambuf class used by DeflatingInputStream and DeflatingOutputStream.
   The actual work is delegated to zlib (see http://zlib.net).
   Both zlib (deflate) streams and gzip streams are supported.
   Output streams should always call close() to ensure
   proper completion of compression.
   A compression level (0 to 9) can be specified in the constructor.
   */
  class deflating_stream_buf : public buffered_stream_buf {
    public:
      /// Creates a DeflatingStreamBuf for compressing data read
      /// from the given input stream.
      deflating_stream_buf (std::istream& istr, compression_type_t type, compression_level_t level);

      /// Creates a DeflatingStreamBuf for compressing data passed
      /// through and forwarding it to the given output stream.
      deflating_stream_buf (std::ostream& ostr, compression_type_t type, compression_level_t level);


      ~deflating_stream_buf () override;

      /// Finishes up the stream.
      ///
      /// Must be called when deflating to an output stream.

      int close ();

    protected:
      int read_from_device (char* buffer, std::streamsize length) override;
      int write_to_device (const char* buffer, std::streamsize length) override;
      int sync () override;
    private:
      struct impl;
      spimpl::unique_impl_ptr<impl> m_pimpl;

  };

  /// The base class for DeflatingOutputStream and DeflatingInputStream.
  ///
  /// This class is needed to ensure the correct initialization
  /// order of the stream buffer and base classes.
  class deflating_ios : public virtual std::ios {
    public:
      /// Creates a DeflatingIOS for compressing data passed
      /// through and forwarding it to the given output stream.
      explicit deflating_ios (std::ostream& ostr,
                              compression_type_t type = compression_type_t::ZLIB,
                              compression_level_t level = compression_level_t::NORMAL);


      /// Creates a DeflatingIOS for compressing data read
      /// from the given input stream.
      explicit deflating_ios (std::istream& istr,
                              compression_type_t type = compression_type_t::ZLIB,
                              compression_level_t level = compression_level_t::NORMAL);

      ~deflating_ios () override;

      deflating_stream_buf* rdbuf ();
    protected:
      deflating_stream_buf m_buf;
  };

  /// This stream compresses all data passing through it
  /// using zlib's deflate algorithm.
  /// After all data has been written to the stream, close()
  /// must be called to ensure completion of compression.
  /// Example:
  ///     std::ofstream ostr("data.gz", std::ios::binary);
  ///     DeflatingOutputStream deflater(ostr, DeflatingStreamBuf::STREAM_GZIP);
  ///     deflater << "Hello, world!" << std::endl;
  ///     deflater.close();
  ///     ostr.close();
  class deflating_output_stream : public std::ostream, public deflating_ios {
    public:
      /// Creates a DeflatingOutputStream for compressing data passed
      /// through and forwarding it to the given output stream.

      explicit deflating_output_stream (std::ostream& ostr,
                                        compression_type_t type = compression_type_t::ZLIB,
                                        compression_level_t level = compression_level_t::NORMAL);


      ~deflating_output_stream () override;


      /// Finishes up the stream.
      ///
      /// Must be called when deflating to an output stream.

      int close ();

    protected:
      virtual int sync ();
  };

  /// This stream compresses all data passing through it
  /// using zlib's deflate algorithm.
  class deflating_input_stream : public std::istream, public deflating_ios {
    public:
      /// Creates a DeflatingIOS for compressing data read
      /// from the given input stream.
      explicit deflating_input_stream (std::istream& istr,
                                       compression_type_t type = compression_type_t::ZLIB,
                                       compression_level_t level = compression_level_t::NORMAL);

      ~deflating_input_stream () override;
  };

}

#endif
