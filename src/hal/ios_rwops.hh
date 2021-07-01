//
// Created by igor on 28/06/2021.
//

#ifndef NEUTRINO_IOS_RWOPS_HH
#define NEUTRINO_IOS_RWOPS_HH

#include <istream>
#include <ostream>

#include <hal/sdl/io.hh>

namespace neutrino::hal
{

    class istream_wrapper : public sdl::rwops_base<istream_wrapper>
    {
    public:
        explicit istream_wrapper(std::istream* is)
                : m_stream(is)
        {}

        uint64_t seek(int offset, sdl::whence whence)
        {
            if (whence == sdl::whence::SET)
            {
                m_stream->seekg(offset, std::ios::beg);
            } else
            {
                if (whence == sdl::whence::CUR)
                {
                    m_stream->seekg(offset, std::ios::cur);
                } else
                {
                    if (whence == sdl::whence::END)
                    {
                        m_stream->seekg(offset, std::ios::end);
                    }
                }
            }

            return m_stream->fail() ? (uint64_t) -1 : (uint64_t) m_stream->tellg();
        }

        std::size_t read(void* ptr, std::size_t size, std::size_t maxnum)
        {
            if (size == 0)
            { return -1; }

            m_stream->read((char*) ptr, size * maxnum);

            return m_stream->bad() ? -1 : m_stream->gcount() / size;
        }

        uint64_t tell()
        {
            return m_stream->tellg();
        }

    private:
        std::istream* m_stream;
    };

    class ostream_wrapper : public sdl::rwops_base<ostream_wrapper>
    {
    public:
        explicit ostream_wrapper(std::ostream* os)
                : m_stream(os)
        {}

        uint64_t seek(int offset, sdl::whence whence)
        {
            if (whence == sdl::whence::SET)
            {
                m_stream->seekp(offset, std::ios::beg);
            } else
            {
                if (whence == sdl::whence::CUR)
                {
                    m_stream->seekp(offset, std::ios::cur);
                } else
                {
                    if (whence == sdl::whence::END)
                    {
                        m_stream->seekp(offset, std::ios::end);
                    }
                }
            }

            return m_stream->fail() ? (uint64_t) -1 : (uint64_t) m_stream->tellp();
        }

        std::size_t write(const void* ptr, std::size_t size, std::size_t maxnum)
        {
            if (size == 0)
            { return -1; }

            m_stream->write((char*) ptr, size * maxnum);

            return m_stream->bad() ? (uint64_t) -1 : (uint64_t) m_stream->tellp() / size;
        }

        uint64_t tell()
        {
            return m_stream->tellp();
        }

    private:
        std::ostream* m_stream;
    };
}

#endif //NEUTRINO_IOS_RWOPS_HH
