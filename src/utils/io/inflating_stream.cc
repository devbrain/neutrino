//
// Created by igor on 08/07/2021.
//

#include <vector>
#include <neutrino/utils/io/inflating_stream.hh>
#include <neutrino/utils/exception.hh>

#include "zstd_wrapper.h"
#include "ios_init.hh"

namespace neutrino::utils::io
{

    static constexpr auto STREAM_BUFFER_SIZE = 1024;
    static constexpr auto INFLATE_BUFFER_SIZE = 32768;

    struct inflating_stream_buf::impl
    {

        impl(std::istream& istr, type_t type)
                : m_istr(&istr),
                  m_ostr(nullptr),
                  m_eof(false),
                  m_check((type != STREAM_ZIP) && (type!=STREAM_ZSTD)),
                  m_buffer(INFLATE_BUFFER_SIZE),
                  m_type(type)
        {
            m_zstream.next_in = nullptr;
            m_zstream.avail_in = 0;
            m_zstream.total_in = 0;
            m_zstream.next_out = nullptr;
            m_zstream.avail_out = 0;
            m_zstream.total_out = 0;
            m_zstream.msg = nullptr;
            m_zstream.state = nullptr;
            m_zstream.zalloc = Z_NULL;
            m_zstream.zfree = Z_NULL;
            m_zstream.opaque = Z_NULL;
            m_zstream.data_type = 0;
            m_zstream.adler = 0;
            m_zstream.reserved = 0;

            int rc = (m_type == STREAM_ZSTD) ?
                     zstd_inflate_init(&m_zstream, "", 0) :
                     inflateInit2(&m_zstream, 15 + (type == STREAM_GZIP ? 16 : 0));

            if (rc != Z_OK)
            {
                RAISE_EX(zError(rc));
            }
        }

        impl(std::istream& istr, int windowBits)
                : m_istr(&istr),
                  m_ostr(nullptr),
                  m_eof(false),
                  m_check(false),
                  m_buffer(INFLATE_BUFFER_SIZE),
                  m_type(STREAM_GZIP)
        {
            m_zstream.zalloc = Z_NULL;
            m_zstream.zfree = Z_NULL;
            m_zstream.opaque = Z_NULL;
            m_zstream.next_in = nullptr;
            m_zstream.avail_in = 0;
            m_zstream.next_out = nullptr;
            m_zstream.avail_out = 0;

            int rc = (m_type == STREAM_ZSTD) ?
                     zstd_inflate_init(&m_zstream, "", 0) :
                     inflateInit2(&m_zstream, windowBits);

            if (rc != Z_OK)
            {
                RAISE_EX(zError(rc));
            }
        }

        impl(std::ostream& ostr, type_t type)
                : m_istr(nullptr),
                  m_ostr(&ostr),
                  m_eof(false),
                  m_check(type != STREAM_ZIP && type != STREAM_ZSTD),
                  m_buffer(INFLATE_BUFFER_SIZE),
                  m_type(type)
        {
            m_zstream.zalloc = Z_NULL;
            m_zstream.zfree = Z_NULL;
            m_zstream.opaque = Z_NULL;
            m_zstream.next_in = nullptr;
            m_zstream.avail_in = 0;
            m_zstream.next_out = nullptr;
            m_zstream.avail_out = 0;

            int rc = (m_type == STREAM_ZSTD) ?
                     zstd_inflate_init(&m_zstream, "", 0) :
                     inflateInit2(&m_zstream, 15 + (type == STREAM_GZIP ? 16 : 0));

            if (rc != Z_OK)
            {
                RAISE_EX(zError(rc));
            }
        }

        impl(std::ostream& ostr, int windowBits)
                : m_istr(nullptr),
                  m_ostr(&ostr),
                  m_eof(false),
                  m_check(false),
                  m_buffer(INFLATE_BUFFER_SIZE),
                  m_type(STREAM_GZIP)
        {
            m_zstream.zalloc = Z_NULL;
            m_zstream.zfree = Z_NULL;
            m_zstream.opaque = Z_NULL;
            m_zstream.next_in = nullptr;
            m_zstream.avail_in = 0;
            m_zstream.next_out = nullptr;
            m_zstream.avail_out = 0;

            int rc = (m_type == STREAM_ZSTD) ?
                     zstd_inflate_init(&m_zstream, "", 0) :
                     inflateInit2(&m_zstream, windowBits);
            if (rc != Z_OK)
            {
                RAISE_EX(zError(rc));
            }
        }

        int do_inflate (z_streamp strm, int flush) {
            return (m_type == STREAM_ZSTD) ?
                    zstd_inflate(strm, flush) :
                    inflate(strm, flush);
        }

        ~impl() {
            if (m_type == STREAM_ZSTD) {
                zstd_inflate_end(&m_zstream);
            }
        }

        std::istream* m_istr;
        std::ostream* m_ostr;

        z_stream m_zstream{};
        bool m_eof;
        bool m_check;
        std::vector<char> m_buffer;
        type_t m_type;
    };

    inflating_stream_buf::inflating_stream_buf(std::istream& istr, type_t type)
            :
            buffered_stream_buf(STREAM_BUFFER_SIZE, std::ios::in),
            m_pimpl(spimpl::make_unique_impl<inflating_stream_buf::impl>(istr, type))
    {

    }

    inflating_stream_buf::inflating_stream_buf(std::istream& istr, int windowBits)
            :
            buffered_stream_buf(STREAM_BUFFER_SIZE, std::ios::in),
            m_pimpl(spimpl::make_unique_impl<inflating_stream_buf::impl>(istr, windowBits))
    {
    }

    inflating_stream_buf::inflating_stream_buf(std::ostream& ostr, type_t type)
            :
            buffered_stream_buf(STREAM_BUFFER_SIZE, std::ios::out),
            m_pimpl(spimpl::make_unique_impl<inflating_stream_buf::impl>(ostr, type))
    {
    }

    inflating_stream_buf::inflating_stream_buf(std::ostream& ostr, int windowBits)
            :
            buffered_stream_buf(STREAM_BUFFER_SIZE, std::ios::out),
            m_pimpl(spimpl::make_unique_impl<inflating_stream_buf::impl>(ostr, windowBits))
    {

    }

    inflating_stream_buf::~inflating_stream_buf()
    {
        try
        {
            close();
        }
        catch (...)
        {
        }
        if (m_pimpl->m_type != STREAM_ZSTD)
        {
            inflateEnd(&m_pimpl->m_zstream);
        }
        else
        {
            zstd_inflate_end(&m_pimpl->m_zstream);
        }
    }

    int inflating_stream_buf::close()
    {
        sync();
        m_pimpl->m_istr = nullptr;
        m_pimpl->m_ostr = nullptr;
        return 0;
    }

    void inflating_stream_buf::reset()
    {
        int rc = (m_pimpl->m_type != STREAM_ZSTD) ?
                inflateReset(&m_pimpl->m_zstream) :
                 zstd_inflate_reset(&m_pimpl->m_zstream);

        if (rc == Z_OK)
        {
            m_pimpl->m_eof = false;
        } else
        {
            RAISE_EX(zError(rc));
        }
    }

    int inflating_stream_buf::read_from_device(char* buffer, std::streamsize length)
    {
        if (m_pimpl->m_eof || !m_pimpl->m_istr)
        { return 0; }

        if (m_pimpl->m_zstream.avail_in == 0)
        {
            int n = 0;
            if (m_pimpl->m_istr->good())
            {
                m_pimpl->m_istr->read(m_pimpl->m_buffer.data(), INFLATE_BUFFER_SIZE);
                n = static_cast<int>(m_pimpl->m_istr->gcount());
            }
            m_pimpl->m_zstream.next_in = (unsigned char*) m_pimpl->m_buffer.data();
            m_pimpl->m_zstream.avail_in = n;
        }
        m_pimpl->m_zstream.next_out = (unsigned char*) buffer;
        m_pimpl->m_zstream.avail_out = static_cast<unsigned>(length);
        for (;;)
        {
            int rc = m_pimpl->do_inflate(&m_pimpl->m_zstream, Z_NO_FLUSH);
            if (rc == Z_DATA_ERROR && !m_pimpl->m_check)
            {
                if (m_pimpl->m_zstream.avail_in == 0)
                {
                    if (m_pimpl->m_istr->good())
                    {
                        rc = Z_OK;
                    } else
                    {
                        rc = Z_STREAM_END;
                    }
                }
            }
            if (rc == Z_STREAM_END)
            {
                m_pimpl->m_eof = true;
                return static_cast<int>(length) - m_pimpl->m_zstream.avail_out;
            }
            if (rc != Z_OK)
            {
                RAISE_EX(zError(rc));
            }
            if (m_pimpl->m_zstream.avail_out == 0)
            {
                return static_cast<int>(length);
            }
            if (m_pimpl->m_zstream.avail_in == 0)
            {
                int n = 0;
                if (m_pimpl->m_istr->good())
                {
                    m_pimpl->m_istr->read(m_pimpl->m_buffer.data(), INFLATE_BUFFER_SIZE);
                    n = static_cast<int>(m_pimpl->m_istr->gcount());
                }
                if (n > 0)
                {
                    m_pimpl->m_zstream.next_in = (unsigned char*) m_pimpl->m_buffer.data();
                    m_pimpl->m_zstream.avail_in = n;
                } else
                { return static_cast<int>(length) - m_pimpl->m_zstream.avail_out; }
            }
        }
    }

    int inflating_stream_buf::write_to_device(const char* buffer, std::streamsize length)
    {
        if (length == 0 || !m_pimpl->m_ostr)
        { return 0; }

        m_pimpl->m_zstream.next_in = (unsigned char*) buffer;
        m_pimpl->m_zstream.avail_in = static_cast<unsigned>(length);
        m_pimpl->m_zstream.next_out = (unsigned char*) m_pimpl->m_buffer.data();
        m_pimpl->m_zstream.avail_out = INFLATE_BUFFER_SIZE;
        for (;;)
        {
            int rc = m_pimpl->do_inflate(&m_pimpl->m_zstream, Z_NO_FLUSH);
            if (rc == Z_STREAM_END)
            {
                m_pimpl->m_ostr->write(m_pimpl->m_buffer.data(), INFLATE_BUFFER_SIZE - m_pimpl->m_zstream.avail_out);
                if (!m_pimpl->m_ostr->good())
                {
                    RAISE_EX("Failed writing inflated data to output stream");
                }
                break;
            }
            if (rc != Z_OK)
            {
                RAISE_EX(zError(rc));
            }
            if (m_pimpl->m_zstream.avail_out == 0)
            {
                m_pimpl->m_ostr->write(m_pimpl->m_buffer.data(), INFLATE_BUFFER_SIZE);
                if (!m_pimpl->m_ostr->good())
                {
                    RAISE_EX("Failed writing inflated data to output stream");
                }
                m_pimpl->m_zstream.next_out = (unsigned char*) m_pimpl->m_buffer.data();
                m_pimpl->m_zstream.avail_out = INFLATE_BUFFER_SIZE;
            }
            if (m_pimpl->m_zstream.avail_in == 0)
            {
                m_pimpl->m_ostr->write(m_pimpl->m_buffer.data(), INFLATE_BUFFER_SIZE - m_pimpl->m_zstream.avail_out);
                if (!m_pimpl->m_ostr->good())
                {
                    RAISE_EX("Failed writing inflated data to output stream");
                }
                m_pimpl->m_zstream.next_out = (unsigned char*) m_pimpl->m_buffer.data();
                m_pimpl->m_zstream.avail_out = INFLATE_BUFFER_SIZE;
                break;
            }
        }
        return static_cast<int>(length);
    }

    int inflating_stream_buf::sync()
    {
        int n = buffered_stream_buf::sync();
        if (!n && m_pimpl->m_ostr)
        { m_pimpl->m_ostr->flush(); }
        return n;
    }

    inflating_ios::inflating_ios(std::ostream& ostr, inflating_stream_buf::type_t type)
            :
            _buf(ostr, type)
    {
        te_ios_init(&_buf);
    }

    inflating_ios::inflating_ios(std::ostream& ostr, int windowBits)
            :
            _buf(ostr, windowBits)
    {
        te_ios_init(&_buf);
    }

    inflating_ios::inflating_ios(std::istream& istr, inflating_stream_buf::type_t type)
            :
            _buf(istr, type)
    {
        te_ios_init(&_buf);
    }

    inflating_ios::inflating_ios(std::istream& istr, int windowBits)
            :
            _buf(istr, windowBits)
    {
        te_ios_init(&_buf);
    }

    inflating_ios::~inflating_ios() = default;

    inflating_stream_buf* inflating_ios::rdbuf()
    {
        return &_buf;
    }

    inflating_output_stream::inflating_output_stream(std::ostream& ostr, inflating_stream_buf::type_t type)
            :
            std::ostream(&_buf),
            inflating_ios(ostr, type)
    {
    }

    inflating_output_stream::inflating_output_stream(std::ostream& ostr, int windowBits)
            :
            std::ostream(&_buf),
            inflating_ios(ostr, windowBits)
    {
    }

    inflating_output_stream::~inflating_output_stream() = default;

    int inflating_output_stream::close()
    {
        return _buf.close();
    }

    inflating_input_stream::inflating_input_stream(std::istream& istr, inflating_stream_buf::type_t type)
            :
            std::istream(&_buf),
            inflating_ios(istr, type)
    {
    }

    inflating_input_stream::inflating_input_stream(std::istream& istr, int windowBits)
            :
            std::istream(&_buf),
            inflating_ios(istr, windowBits)
    {
    }

    inflating_input_stream::~inflating_input_stream() = default;

    void inflating_input_stream::reset()
    {
        _buf.reset();
        clear();
    }
}