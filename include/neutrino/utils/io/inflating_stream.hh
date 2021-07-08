//
// Created by igor on 08/07/2021.
//

#ifndef NEUTRINO_UTILS_IO_INFLATING_STREAM_HH
#define NEUTRINO_UTILS_IO_INFLATING_STREAM_HH

#include <ostream>
#include <istream>
#include <neutrino/utils/io/buffered_stream_buf.hh>
#include <zlib.h>

namespace neutrino::utils::io
{
    class inflating_stream_buf : public buffered_stream_buf
        /// This is the streambuf class used by InflatingInputStream and InflatingOutputStream.
        /// The actual work is delegated to zlib (see http://zlib.net).
        /// Both zlib (deflate) streams and gzip streams are supported.
        /// Output streams should always call close() to ensure
        /// proper completion of decompression.
    {
    public:
        enum StreamType
        {
            STREAM_ZLIB, /// Expect a zlib header, use Adler-32 checksum.
            STREAM_GZIP, /// Expect a gzip header, use CRC-32 checksum.
            STREAM_ZIP   /// STREAM_ZIP is handled as STREAM_ZLIB, except that we do not check the ADLER32 value (must be checked by caller)
        };

        inflating_stream_buf(std::istream& istr, StreamType type);
        /// Creates an InflatingStreamBuf for expanding the compressed data read from
        /// the give input stream.

        inflating_stream_buf(std::istream& istr, int windowBits);
        /// Creates an InflatingStreamBuf for expanding the compressed data read from
        /// the given input stream.
        ///
        /// Please refer to the zlib documentation of inflateInit2() for a description
        /// of the windowBits parameter.

        inflating_stream_buf(std::ostream& ostr, StreamType type);
        /// Creates an InflatingStreamBuf for expanding the compressed data passed through
        /// and forwarding it to the given output stream.

        inflating_stream_buf(std::ostream& ostr, int windowBits);
        /// Creates an InflatingStreamBuf for expanding the compressed data passed through
        /// and forwarding it to the given output stream.
        ///
        /// Please refer to the zlib documentation of inflateInit2() for a description
        /// of the windowBits parameter.

        ~inflating_stream_buf() override;
        /// Destroys the InflatingStreamBuf.

        int close();
        /// Finishes up the stream.
        ///
        /// Must be called when inflating to an output stream.

        void reset();
        /// Resets the stream buffer.

    protected:
        int read_from_device(char* buffer, std::streamsize length) override;
        int write_to_device(const char* buffer, std::streamsize length) override;
        int sync() override;

    private:
        enum
        {
            STREAM_BUFFER_SIZE = 1024,
            INFLATE_BUFFER_SIZE = 32768
        };

        std::istream* _pIstr;
        std::ostream* _pOstr;
        char* _buffer;
        z_stream _zstr{};
        bool _eof;
        bool _check;
    };

    class inflating_ios : public virtual std::ios
/// The base class for InflatingOutputStream and InflatingInputStream.
///
/// This class is needed to ensure the correct initialization
/// order of the stream buffer and base classes.
    {
    public:
        explicit inflating_ios(std::ostream& ostr, inflating_stream_buf::StreamType type = inflating_stream_buf::STREAM_ZLIB);
/// Creates an InflatingIOS for expanding the compressed data passed through
/// and forwarding it to the given output stream.

        inflating_ios(std::ostream& ostr, int windowBits);
/// Creates an InflatingIOS for expanding the compressed data passed through
/// and forwarding it to the given output stream.
///
/// Please refer to the zlib documentation of inflateInit2() for a description
/// of the windowBits parameter.

        explicit inflating_ios(std::istream& istr, inflating_stream_buf::StreamType type = inflating_stream_buf::STREAM_ZLIB);
/// Creates an InflatingIOS for expanding the compressed data read from
/// the given input stream.

        inflating_ios(std::istream& istr, int windowBits);
/// Creates an InflatingIOS for expanding the compressed data read from
/// the given input stream.
///
/// Please refer to the zlib documentation of inflateInit2() for a description
/// of the windowBits parameter.

        ~inflating_ios() override;
/// Destroys the InflatingIOS.

        inflating_stream_buf* rdbuf();
/// Returns a pointer to the underlying stream buffer.

    protected:
        inflating_stream_buf _buf;
    };

    class inflating_output_stream : public std::ostream, public inflating_ios
/// This stream decompresses all data passing through it
/// using zlib's inflate algorithm.
///
/// After all data has been written to the stream, close()
/// must be called to ensure completion of decompression.
    {
    public:
        explicit inflating_output_stream(std::ostream& ostr,
                                         inflating_stream_buf::StreamType type = inflating_stream_buf::STREAM_ZLIB);
/// Creates an InflatingOutputStream for expanding the compressed data passed through
/// and forwarding it to the given output stream.

        inflating_output_stream(std::ostream& ostr, int windowBits);
/// Creates an InflatingOutputStream for expanding the compressed data passed through
/// and forwarding it to the given output stream.
///
/// Please refer to the zlib documentation of inflateInit2() for a description
/// of the windowBits parameter.

        ~inflating_output_stream() override;
/// Destroys the InflatingOutputStream.

        int close();
/// Finishes up the stream.
///
/// Must be called to ensure all data is properly written to
/// the target output stream.
    };

    class inflating_input_stream : public std::istream, public inflating_ios
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
    {
    public:
        explicit inflating_input_stream(std::istream& istr, inflating_stream_buf::StreamType type = inflating_stream_buf::STREAM_ZLIB);
/// Creates an InflatingInputStream for expanding the compressed data read from
/// the given input stream.

        inflating_input_stream(std::istream& istr, int windowBits);
/// Creates an InflatingInputStream for expanding the compressed data read from
/// the given input stream.
///
/// Please refer to the zlib documentation of inflateInit2() for a description
/// of the windowBits parameter.

        ~inflating_input_stream() override;
/// Destroys the InflatingInputStream.

        void reset();
/// Resets the zlib machinery so that another zlib stream can be read from
/// the same underlying input stream.
    };


}

#endif
