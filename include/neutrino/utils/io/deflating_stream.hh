//
// Created by igor on 08/07/2021.
//

#ifndef NEUTRINO_UTILS_IO_ZLIB_OSTREAM_HH
#define NEUTRINO_UTILS_IO_ZLIB_OSTREAM_HH

#include <ostream>
#include <istream>
#include <neutrino/utils/io/buffered_stream_buf.hh>
#include <zlib.h>

namespace neutrino::utils::io
{
    class deflating_stream_buf : public buffered_stream_buf
        /// This is the streambuf class used by DeflatingInputStream and DeflatingOutputStream.
        /// The actual work is delegated to zlib (see http://zlib.net).
        /// Both zlib (deflate) streams and gzip streams are supported.
        /// Output streams should always call close() to ensure
        /// proper completion of compression.
        /// A compression level (0 to 9) can be specified in the constructor.
    {
    public:
        enum StreamType
        {
            STREAM_ZLIB, /// Create a zlib header, use Adler-32 checksum.
            STREAM_GZIP  /// Create a gzip header, use CRC-32 checksum.
        };

        deflating_stream_buf(std::istream& istr, StreamType type, int level);
        /// Creates a DeflatingStreamBuf for compressing data read
        /// from the given input stream.

        deflating_stream_buf(std::istream& istr, int windowBits, int level);
        /// Creates a DeflatingStreamBuf for compressing data read
        /// from the given input stream.
        ///
        /// Please refer to the zlib documentation of deflateInit2() for a description
        /// of the windowBits parameter.

        deflating_stream_buf(std::ostream& ostr, StreamType type, int level);
        /// Creates a DeflatingStreamBuf for compressing data passed
        /// through and forwarding it to the given output stream.

        deflating_stream_buf(std::ostream& ostr, int windowBits, int level);
        /// Creates a DeflatingStreamBuf for compressing data passed
        /// through and forwarding it to the given output stream.
        ///
        /// Please refer to the zlib documentation of deflateInit2() for a description
        /// of the windowBits parameter.

        ~deflating_stream_buf() override;
        /// Destroys the DeflatingStreamBuf.

        int close();
        /// Finishes up the stream.
        ///
        /// Must be called when deflating to an output stream.

    protected:
        int read_from_device(char* buffer, std::streamsize length) override;
        int write_to_device(const char* buffer, std::streamsize length) override;
        int sync() override;

    private:
        enum
        {
            STREAM_BUFFER_SIZE = 1024,
            DEFLATE_BUFFER_SIZE = 32768
        };

        std::istream* _pIstr;
        std::ostream* _pOstr;
        char* _buffer;
        z_stream _zstr;
        bool _eof;
    };

    class deflating_ios : public virtual std::ios
/// The base class for DeflatingOutputStream and DeflatingInputStream.
///
/// This class is needed to ensure the correct initialization
/// order of the stream buffer and base classes.
    {
    public:
        explicit deflating_ios(std::ostream& ostr, deflating_stream_buf::StreamType type = deflating_stream_buf::STREAM_ZLIB,
                               int level = Z_DEFAULT_COMPRESSION);
/// Creates a DeflatingIOS for compressing data passed
/// through and forwarding it to the given output stream.

        deflating_ios(std::ostream& ostr, int windowBits, int level);
/// Creates a DeflatingIOS for compressing data passed
/// through and forwarding it to the given output stream.
///
/// Please refer to the zlib documentation of deflateInit2() for a description
/// of the windowBits parameter.

        explicit deflating_ios(std::istream& istr, deflating_stream_buf::StreamType type = deflating_stream_buf::STREAM_ZLIB,
                               int level = Z_DEFAULT_COMPRESSION);
/// Creates a DeflatingIOS for compressing data read
/// from the given input stream.

        deflating_ios(std::istream& istr, int windowBits, int level);
/// Creates a DeflatingIOS for compressing data read
/// from the given input stream.
///
/// Please refer to the zlib documentation of deflateInit2() for a description
/// of the windowBits parameter.

        ~deflating_ios() override;
/// Destroys the DeflatingIOS.

        deflating_stream_buf* rdbuf();
/// Returns a pointer to the underlying stream buffer.

    protected:
        deflating_stream_buf _buf;
    };

    class deflating_output_stream : public std::ostream, public deflating_ios
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
    {
    public:
        explicit deflating_output_stream(std::ostream& ostr, deflating_stream_buf::StreamType type = deflating_stream_buf::STREAM_ZLIB,
                                         int level = Z_DEFAULT_COMPRESSION);
/// Creates a DeflatingOutputStream for compressing data passed
/// through and forwarding it to the given output stream.

        deflating_output_stream(std::ostream& ostr, int windowBits, int level);
/// Creates a DeflatingOutputStream for compressing data passed
/// through and forwarding it to the given output stream.
///
/// Please refer to the zlib documentation of deflateInit2() for a description
/// of the windowBits parameter.

        ~deflating_output_stream() override;
/// Destroys the DeflatingOutputStream.

        int close();
/// Finishes up the stream.
///
/// Must be called when deflating to an output stream.

    protected:
        virtual int sync();
    };

    class deflating_input_stream

            : public std::istream, public deflating_ios
/// This stream compresses all data passing through it
/// using zlib's deflate algorithm.
    {
    public:
        explicit deflating_input_stream(std::istream
                             & istr,
                                        deflating_stream_buf::StreamType type = deflating_stream_buf::STREAM_ZLIB,
                                        int level = Z_DEFAULT_COMPRESSION
        );
/// Creates a DeflatingIOS for compressing data read
/// from the given input stream.

        deflating_input_stream(std::istream
                             & istr,
                               int windowBits,
                               int level
        );
/// Creates a DeflatingIOS for compressing data read
/// from the given input stream.
///
/// Please refer to the zlib documentation of deflateInit2() for a description
/// of the windowBits parameter.

        ~deflating_input_stream() override;
/// Destroys the DeflatingInputStream.
    };

}

#endif
