//
// Created by igor on 08/07/2021.
//

#include <neutrino/utils/io/inflating_stream.hh>
#include <neutrino/utils/exception.hh>
#include "ios_init.hh"

namespace neutrino::utils::io {
    InflatingStreamBuf::InflatingStreamBuf(std::istream& istr, StreamType type):
            buffered_stream_buf(STREAM_BUFFER_SIZE, std::ios::in),
            _pIstr(&istr),
            _pOstr(nullptr),
            _eof(false),
            _check(type != STREAM_ZIP)
    {
        _zstr.next_in   = nullptr;
        _zstr.avail_in  = 0;
        _zstr.total_in  = 0;
        _zstr.next_out  = nullptr;
        _zstr.avail_out = 0;
        _zstr.total_out = 0;
        _zstr.msg       = nullptr;
        _zstr.state     = nullptr;
        _zstr.zalloc    = Z_NULL;
        _zstr.zfree     = Z_NULL;
        _zstr.opaque    = Z_NULL;
        _zstr.data_type = 0;
        _zstr.adler     = 0;
        _zstr.reserved  = 0;

        _buffer = new char[INFLATE_BUFFER_SIZE];

        int rc = inflateInit2(&_zstr, 15 + (type == STREAM_GZIP ? 16 : 0));
        if (rc != Z_OK)
        {
            delete [] _buffer;
            RAISE_EX(zError(rc));
        }
    }


    InflatingStreamBuf::InflatingStreamBuf(std::istream& istr, int windowBits):
            buffered_stream_buf(STREAM_BUFFER_SIZE, std::ios::in),
            _pIstr(&istr),
            _pOstr(nullptr),
            _eof(false),
            _check(false)
    {
        _zstr.zalloc    = Z_NULL;
        _zstr.zfree     = Z_NULL;
        _zstr.opaque    = Z_NULL;
        _zstr.next_in   = nullptr;
        _zstr.avail_in  = 0;
        _zstr.next_out  = nullptr;
        _zstr.avail_out = 0;

        _buffer = new char[INFLATE_BUFFER_SIZE];

        int rc = inflateInit2(&_zstr, windowBits);
        if (rc != Z_OK)
        {
            delete [] _buffer;

            RAISE_EX(zError(rc));
        }
    }


    InflatingStreamBuf::InflatingStreamBuf(std::ostream& ostr, StreamType type):
            buffered_stream_buf(STREAM_BUFFER_SIZE, std::ios::out),
            _pIstr(nullptr),
            _pOstr(&ostr),
            _eof(false),
            _check(type != STREAM_ZIP)
    {
        _zstr.zalloc    = Z_NULL;
        _zstr.zfree     = Z_NULL;
        _zstr.opaque    = Z_NULL;
        _zstr.next_in   = nullptr;
        _zstr.avail_in  = 0;
        _zstr.next_out  = nullptr;
        _zstr.avail_out = 0;

        _buffer = new char[INFLATE_BUFFER_SIZE];

        int rc = inflateInit2(&_zstr, 15 + (type == STREAM_GZIP ? 16 : 0));
        if (rc != Z_OK)
        {
            delete [] _buffer;
            RAISE_EX(zError(rc));
        }
    }


    InflatingStreamBuf::InflatingStreamBuf(std::ostream& ostr, int windowBits):
            buffered_stream_buf(STREAM_BUFFER_SIZE, std::ios::out),
            _pIstr(nullptr),
            _pOstr(&ostr),
            _eof(false),
            _check(false)
    {
        _zstr.zalloc    = Z_NULL;
        _zstr.zfree     = Z_NULL;
        _zstr.opaque    = Z_NULL;
        _zstr.next_in   = nullptr;
        _zstr.avail_in  = 0;
        _zstr.next_out  = nullptr;
        _zstr.avail_out = 0;

        _buffer = new char[INFLATE_BUFFER_SIZE];

        int rc = inflateInit2(&_zstr, windowBits);
        if (rc != Z_OK)
        {
            delete [] _buffer;
            RAISE_EX(zError(rc));
        }
    }


    InflatingStreamBuf::~InflatingStreamBuf()
    {
        try
        {
            close();
        }
        catch (...)
        {
        }
        delete [] _buffer;
        inflateEnd(&_zstr);
    }


    int InflatingStreamBuf::close()
    {
        sync();
        _pIstr = nullptr;
        _pOstr = nullptr;
        return 0;
    }


    void InflatingStreamBuf::reset()
    {
        int rc = inflateReset(&_zstr);
        if (rc == Z_OK)
        {
            _eof = false;
        }
        else
        {
            RAISE_EX(zError(rc));
        }
    }


    int InflatingStreamBuf::read_from_device(char* buffer, std::streamsize length)
    {
        if (_eof || !_pIstr) return 0;

        if (_zstr.avail_in == 0)
        {
            int n = 0;
            if (_pIstr->good())
            {
                _pIstr->read(_buffer, INFLATE_BUFFER_SIZE);
                n = static_cast<int>(_pIstr->gcount());
            }
            _zstr.next_in   = (unsigned char*) _buffer;
            _zstr.avail_in  = n;
        }
        _zstr.next_out  = (unsigned char*) buffer;
        _zstr.avail_out = static_cast<unsigned>(length);
        for (;;)
        {
            int rc = inflate(&_zstr, Z_NO_FLUSH);
            if (rc == Z_DATA_ERROR && !_check)
            {
                if (_zstr.avail_in == 0)
                {
                    if (_pIstr->good())
                        rc = Z_OK;
                    else
                        rc = Z_STREAM_END;
                }
            }
            if (rc == Z_STREAM_END)
            {
                _eof = true;
                return static_cast<int>(length) - _zstr.avail_out;
            }
            if (rc != Z_OK) {
                RAISE_EX(zError(rc));
            }
            if (_zstr.avail_out == 0)
                return static_cast<int>(length);
            if (_zstr.avail_in == 0)
            {
                int n = 0;
                if (_pIstr->good())
                {
                    _pIstr->read(_buffer, INFLATE_BUFFER_SIZE);
                    n = static_cast<int>(_pIstr->gcount());
                }
                if (n > 0)
                {
                    _zstr.next_in  = (unsigned char*) _buffer;
                    _zstr.avail_in = n;
                }
                else return static_cast<int>(length) - _zstr.avail_out;
            }
        }
    }


    int InflatingStreamBuf::write_to_device(const char* buffer, std::streamsize length)
    {
        if (length == 0 || !_pOstr) return 0;

        _zstr.next_in   = (unsigned char*) buffer;
        _zstr.avail_in  = static_cast<unsigned>(length);
        _zstr.next_out  = (unsigned char*) _buffer;
        _zstr.avail_out = INFLATE_BUFFER_SIZE;
        for (;;)
        {
            int rc = inflate(&_zstr, Z_NO_FLUSH);
            if (rc == Z_STREAM_END)
            {
                _pOstr->write(_buffer, INFLATE_BUFFER_SIZE - _zstr.avail_out);
                if (!_pOstr->good()) {
                    RAISE_EX("Failed writing inflated data to output stream");
                }
                break;
            }
            if (rc != Z_OK) {
                RAISE_EX(zError(rc));
            }
            if (_zstr.avail_out == 0)
            {
                _pOstr->write(_buffer, INFLATE_BUFFER_SIZE);
                if (!_pOstr->good()) {
                    RAISE_EX("Failed writing inflated data to output stream");
                }
                _zstr.next_out  = (unsigned char*) _buffer;
                _zstr.avail_out = INFLATE_BUFFER_SIZE;
            }
            if (_zstr.avail_in == 0)
            {
                _pOstr->write(_buffer, INFLATE_BUFFER_SIZE - _zstr.avail_out);
                if (!_pOstr->good()) {
                    RAISE_EX("Failed writing inflated data to output stream");
                }
                _zstr.next_out  = (unsigned char*) _buffer;
                _zstr.avail_out = INFLATE_BUFFER_SIZE;
                break;
            }
        }
        return static_cast<int>(length);
    }


    int InflatingStreamBuf::sync()
    {
        int n = buffered_stream_buf::sync();
        if (!n && _pOstr) _pOstr->flush();
        return n;
    }


    InflatingIOS::InflatingIOS(std::ostream& ostr, InflatingStreamBuf::StreamType type):
            _buf(ostr, type)
    {
        te_ios_init(&_buf);
    }


    InflatingIOS::InflatingIOS(std::ostream& ostr, int windowBits):
            _buf(ostr, windowBits)
    {
        te_ios_init(&_buf);
    }


    InflatingIOS::InflatingIOS(std::istream& istr, InflatingStreamBuf::StreamType type):
            _buf(istr, type)
    {
        te_ios_init(&_buf);
    }


    InflatingIOS::InflatingIOS(std::istream& istr, int windowBits):
            _buf(istr, windowBits)
    {
        te_ios_init(&_buf);
    }


    InflatingIOS::~InflatingIOS() = default;


    InflatingStreamBuf* InflatingIOS::rdbuf()
    {
        return &_buf;
    }


    InflatingOutputStream::InflatingOutputStream(std::ostream& ostr, InflatingStreamBuf::StreamType type):
            std::ostream(&_buf),
            InflatingIOS(ostr, type)
    {
    }


    InflatingOutputStream::InflatingOutputStream(std::ostream& ostr, int windowBits):
            std::ostream(&_buf),
            InflatingIOS(ostr, windowBits)
    {
    }


    InflatingOutputStream::~InflatingOutputStream()  = default;


    int InflatingOutputStream::close()
    {
        return _buf.close();
    }


    InflatingInputStream::InflatingInputStream(std::istream& istr, InflatingStreamBuf::StreamType type):
            std::istream(&_buf),
            InflatingIOS(istr, type)
    {
    }


    InflatingInputStream::InflatingInputStream(std::istream& istr, int windowBits):
            std::istream(&_buf),
            InflatingIOS(istr, windowBits)
    {
    }


    InflatingInputStream::~InflatingInputStream() = default;


    void InflatingInputStream::reset()
    {
        _buf.reset();
        clear();
    }
}