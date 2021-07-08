//
// Created by igor on 08/07/2021.
//

#include <neutrino/utils/io/deflating_stream.hh>
#include <neutrino/utils/exception.hh>
#include "ios_init.hh"

namespace neutrino::utils::io {
    deflating_stream_buf::deflating_stream_buf(std::istream& istr, StreamType type, int level):
            buffered_stream_buf(STREAM_BUFFER_SIZE, std::ios::in),
            _pIstr(&istr),
            _pOstr(0),
            _eof(false)
    {
        _zstr.next_in   = 0;
        _zstr.avail_in  = 0;
        _zstr.total_in  = 0;
        _zstr.next_out  = 0;
        _zstr.avail_out = 0;
        _zstr.total_out = 0;
        _zstr.msg       = 0;
        _zstr.state     = 0;
        _zstr.zalloc    = Z_NULL;
        _zstr.zfree     = Z_NULL;
        _zstr.opaque    = Z_NULL;
        _zstr.data_type = 0;
        _zstr.adler     = 0;
        _zstr.reserved  = 0;

        _buffer = new char[DEFLATE_BUFFER_SIZE];

        int rc = deflateInit2(&_zstr, level, Z_DEFLATED, 15 + (type == STREAM_GZIP ? 16 : 0), 8, Z_DEFAULT_STRATEGY);
        if (rc != Z_OK)
        {
            delete [] _buffer;
            RAISE_EX(zError(rc));
        }
    }


    deflating_stream_buf::deflating_stream_buf(std::istream& istr, int windowBits, int level):
            buffered_stream_buf(STREAM_BUFFER_SIZE, std::ios::in),
            _pIstr(&istr),
            _pOstr(0),
            _eof(false)
    {
        _zstr.zalloc    = Z_NULL;
        _zstr.zfree     = Z_NULL;
        _zstr.opaque    = Z_NULL;
        _zstr.next_in   = 0;
        _zstr.avail_in  = 0;
        _zstr.next_out  = 0;
        _zstr.avail_out = 0;

        _buffer = new char[DEFLATE_BUFFER_SIZE];

        int rc = deflateInit2(&_zstr, level, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY);
        if (rc != Z_OK)
        {
            delete [] _buffer;
            RAISE_EX(zError(rc));
        }
    }


    deflating_stream_buf::deflating_stream_buf(std::ostream& ostr, StreamType type, int level):
            buffered_stream_buf(STREAM_BUFFER_SIZE, std::ios::out),
            _pIstr(0),
            _pOstr(&ostr),
            _eof(false)
    {
        _zstr.zalloc    = Z_NULL;
        _zstr.zfree     = Z_NULL;
        _zstr.opaque    = Z_NULL;
        _zstr.next_in   = 0;
        _zstr.avail_in  = 0;
        _zstr.next_out  = 0;
        _zstr.avail_out = 0;

        _buffer = new char[DEFLATE_BUFFER_SIZE];

        int rc = deflateInit2(&_zstr, level, Z_DEFLATED, 15 + (type == STREAM_GZIP ? 16 : 0), 8, Z_DEFAULT_STRATEGY);
        if (rc != Z_OK)
        {
            delete [] _buffer;
            RAISE_EX(zError(rc));
        }
    }


    deflating_stream_buf::deflating_stream_buf(std::ostream& ostr, int windowBits, int level):
            buffered_stream_buf(STREAM_BUFFER_SIZE, std::ios::out),
            _pIstr(0),
            _pOstr(&ostr),
            _eof(false)
    {
        _zstr.zalloc    = Z_NULL;
        _zstr.zfree     = Z_NULL;
        _zstr.opaque    = Z_NULL;
        _zstr.next_in   = 0;
        _zstr.avail_in  = 0;
        _zstr.next_out  = 0;
        _zstr.avail_out = 0;

        _buffer = new char[DEFLATE_BUFFER_SIZE];

        int rc = deflateInit2(&_zstr, level, Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY);
        if (rc != Z_OK)
        {
            delete [] _buffer;
            RAISE_EX(zError(rc));
        }
    }


    deflating_stream_buf::~deflating_stream_buf()
    {
        try
        {
            close();
        }
        catch (...)
        {
        }
        delete [] _buffer;
        deflateEnd(&_zstr);
    }


    int deflating_stream_buf::close()
    {
        buffered_stream_buf ::sync();
        _pIstr = 0;
        if (_pOstr)
        {
            if (_zstr.next_out)
            {
                int rc = deflate(&_zstr, Z_FINISH);
                if (rc != Z_OK && rc != Z_STREAM_END){
                    RAISE_EX(zError(rc));
                };
                _pOstr->write(_buffer, DEFLATE_BUFFER_SIZE - _zstr.avail_out);
                if (!_pOstr->good()) {
                    RAISE_EX("Failed writing deflated data to output stream");
                }
                _zstr.next_out  = (unsigned char*) _buffer;
                _zstr.avail_out = DEFLATE_BUFFER_SIZE;
                while (rc != Z_STREAM_END)
                {
                    rc = deflate(&_zstr, Z_FINISH);
                    if (rc != Z_OK && rc != Z_STREAM_END){
                        RAISE_EX(zError(rc));
                    }
                    _pOstr->write(_buffer, DEFLATE_BUFFER_SIZE - _zstr.avail_out);
                    if (!_pOstr->good()) {
                        RAISE_EX("Failed writing deflated data to output stream");
                    }
                    _zstr.next_out  = (unsigned char*) _buffer;
                    _zstr.avail_out = DEFLATE_BUFFER_SIZE;
                }
            }
            _pOstr->flush();
            _pOstr = 0;
        }
        return 0;
    }


    int deflating_stream_buf::sync()
    {
        if (buffered_stream_buf ::sync())
            return -1;

        if (_pOstr)
        {
            if (_zstr.next_out)
            {
                int rc = deflate(&_zstr, Z_SYNC_FLUSH);
                if (rc != Z_OK) {
                    RAISE_EX(zError(rc));
                };
                _pOstr->write(_buffer, DEFLATE_BUFFER_SIZE - _zstr.avail_out);
                if (!_pOstr->good()) {
                    RAISE_EX("Failed writing deflated data to output stream");
                }
                while (_zstr.avail_out == 0)
                {
                    _zstr.next_out  = (unsigned char*) _buffer;
                    _zstr.avail_out = DEFLATE_BUFFER_SIZE;
                    rc = deflate(&_zstr, Z_SYNC_FLUSH);
                    if (rc != Z_OK) {
                        RAISE_EX(zError(rc));
                    } ;
                    _pOstr->write(_buffer, DEFLATE_BUFFER_SIZE - _zstr.avail_out);
                    if (!_pOstr->good()){
                        RAISE_EX("Failed writing deflated data to output stream");
                    }
                };
                _zstr.next_out  = (unsigned char*) _buffer;
                _zstr.avail_out = DEFLATE_BUFFER_SIZE;
            }
            // NOTE: This breaks the Zip library and causes corruption in some files.
            // See GH #1828
            // _pOstr->flush();
        }
        return 0;
    }


    int deflating_stream_buf::read_from_device(char* buffer, std::streamsize length)
    {
        if (!_pIstr) return 0;
        if (_zstr.avail_in == 0 && !_eof)
        {
            int n = 0;
            if (_pIstr->good())
            {
                _pIstr->read(_buffer, DEFLATE_BUFFER_SIZE);
                n = static_cast<int>(_pIstr->gcount());
            }
            if (n > 0)
            {
                _zstr.next_in  = (unsigned char*) _buffer;
                _zstr.avail_in = n;
            }
            else
            {
                _zstr.next_in  = 0;
                _zstr.avail_in = 0;
                _eof = true;
            }
        }
        _zstr.next_out  = (unsigned char*) buffer;
        _zstr.avail_out = static_cast<unsigned>(length);
        for (;;)
        {
            int rc = deflate(&_zstr, _eof ? Z_FINISH : Z_NO_FLUSH);
            if (_eof && rc == Z_STREAM_END)
            {
                _pIstr = 0;
                return static_cast<int>(length) - _zstr.avail_out;
            }
            if (rc != Z_OK) {
                RAISE_EX(zError(rc));
            }
            if (_zstr.avail_out == 0)
            {
                return static_cast<int>(length);
            }
            if (_zstr.avail_in == 0)
            {
                int n = 0;
                if (_pIstr->good())
                {
                    _pIstr->read(_buffer, DEFLATE_BUFFER_SIZE);
                    n = static_cast<int>(_pIstr->gcount());
                }
                if (n > 0)
                {
                    _zstr.next_in  = (unsigned char*) _buffer;
                    _zstr.avail_in = n;
                }
                else
                {
                    _zstr.next_in  = 0;
                    _zstr.avail_in = 0;
                    _eof = true;
                }
            }
        }
    }


    int deflating_stream_buf::write_to_device(const char* buffer, std::streamsize length)
    {
        if (length == 0 || !_pOstr) return 0;

        _zstr.next_in   = (unsigned char*) buffer;
        _zstr.avail_in  = static_cast<unsigned>(length);
        _zstr.next_out  = (unsigned char*) _buffer;
        _zstr.avail_out = DEFLATE_BUFFER_SIZE;
        for (;;)
        {
            int rc = deflate(&_zstr, Z_NO_FLUSH);
            if (rc != Z_OK) {
                RAISE_EX(zError(rc));
            }
            if (_zstr.avail_out == 0)
            {
                _pOstr->write(_buffer, DEFLATE_BUFFER_SIZE);
                if (!_pOstr->good()) {
                    RAISE_EX("Failed writing deflated data to output stream");
                }
                _zstr.next_out  = (unsigned char*) _buffer;
                _zstr.avail_out = DEFLATE_BUFFER_SIZE;
            }
            if (_zstr.avail_in == 0)
            {
                _pOstr->write(_buffer, DEFLATE_BUFFER_SIZE - _zstr.avail_out);
                if (!_pOstr->good()) {
                    RAISE_EX("Failed writing deflated data to output stream");
                }
                _zstr.next_out  = (unsigned char*) _buffer;
                _zstr.avail_out = DEFLATE_BUFFER_SIZE;
                break;
            }
        }
        return static_cast<int>(length);
    }


    deflating_ios::deflating_ios(std::ostream& ostr, deflating_stream_buf::StreamType type, int level):
            _buf(ostr, type, level)
    {
        te_ios_init(&_buf);
    }


    deflating_ios::deflating_ios(std::ostream& ostr, int windowBits, int level):
            _buf(ostr, windowBits, level)
    {
        te_ios_init(&_buf);
    }


    deflating_ios::deflating_ios(std::istream& istr, deflating_stream_buf::StreamType type, int level):
            _buf(istr, type, level)
    {
        te_ios_init(&_buf);
    }


    deflating_ios::deflating_ios(std::istream& istr, int windowBits, int level):
            _buf(istr, windowBits, level)
    {
        te_ios_init(&_buf);
    }


    deflating_ios::~deflating_ios()
    {
    }


    deflating_stream_buf* deflating_ios::rdbuf()
    {
        return &_buf;
    }


    deflating_output_stream::deflating_output_stream(std::ostream& ostr, deflating_stream_buf::StreamType type, int level):
            std::ostream(&_buf),
            deflating_ios(ostr, type, level)
    {
    }


    deflating_output_stream::deflating_output_stream(std::ostream& ostr, int windowBits, int level):
            std::ostream(&_buf),
            deflating_ios(ostr, windowBits, level)
    {
    }


    deflating_output_stream::~deflating_output_stream()
    {
    }


    int deflating_output_stream::close()
    {
        return _buf.close();
    }


    int deflating_output_stream::sync()
    {
        return _buf.pubsync();
    }


    deflating_input_stream::deflating_input_stream(std::istream& istr, deflating_stream_buf::StreamType type, int level):
            std::istream(&_buf),
            deflating_ios(istr, type, level)
    {
    }


    deflating_input_stream::deflating_input_stream(std::istream& istr, int windowBits, int level):
            std::istream(&_buf),
            deflating_ios(istr, windowBits, level)
    {
    }


    deflating_input_stream::~deflating_input_stream()
    {
    }

}