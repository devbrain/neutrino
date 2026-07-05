//
// Created by igor on 02/07/2026.
//

#include "sdl_io_stream.hh"
#include <failsafe/enforce.hh>

namespace neutrino::audio_detail {

class sdl_io_stream_impl : public musac::io_stream {
public:
    explicit sdl_io_stream_impl(SDL_IOStream* stream)
        : m_stream(stream) {
        ENFORCE(m_stream != nullptr)("Cannot wrap null SDL_IOStream");
    }

    ~sdl_io_stream_impl() override {
        close();
    }

    size_t read(void* ptr, size_t size_bytes) override {
        if (!m_stream) return 0;
        return SDL_ReadIO(m_stream, ptr, size_bytes);
    }

    size_t write(const void* ptr, size_t size_bytes) override {
        if (!m_stream) return 0;
        return SDL_WriteIO(m_stream, ptr, size_bytes);
    }

    int64_t seek(int64_t offset, musac::seek_origin whence) override {
        if (!m_stream) return -1;
        SDL_IOWhence sdl_whence;
        switch (whence) {
            case musac::seek_origin::set: sdl_whence = SDL_IO_SEEK_SET; break;
            case musac::seek_origin::cur: sdl_whence = SDL_IO_SEEK_CUR; break;
            case musac::seek_origin::end: sdl_whence = SDL_IO_SEEK_END; break;
            default: return -1;
        }
        return SDL_SeekIO(m_stream, offset, sdl_whence);
    }

    int64_t tell() override {
        if (!m_stream) return -1;
        return SDL_TellIO(m_stream);
    }

    int64_t get_size() override {
        if (!m_stream) return -1;
        return SDL_GetIOSize(m_stream);
    }

    void close() override {
        if (m_stream) {
            SDL_CloseIO(m_stream);
            m_stream = nullptr;
        }
    }

    bool is_open() const override {
        return m_stream != nullptr;
    }

private:
    SDL_IOStream* m_stream;
};

std::unique_ptr<musac::io_stream> io_from_sdl(SDL_IOStream* sdl_stream) {
    if (!sdl_stream) {
        return nullptr;
    }
    return std::make_unique<sdl_io_stream_impl>(sdl_stream);
}

std::unique_ptr<musac::io_stream> io_from_file(const std::string& path) {
    SDL_IOStream* file = SDL_IOFromFile(path.c_str(), "rb");
    if (!file) {
        return nullptr;
    }
    return std::make_unique<sdl_io_stream_impl>(file);
}

} // namespace neutrino::audio_detail
