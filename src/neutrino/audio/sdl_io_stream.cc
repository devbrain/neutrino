//
// Created by igor on 02/07/2026.
//

#include "sdl_io_stream.hh"
#include <failsafe/enforce.hh>

#include <utility>

namespace neutrino::audio_detail {

class sdl_io_stream_impl : public musac::io_stream {
public:
    explicit sdl_io_stream_impl(sdlpp::iostream stream)
        : m_stream(std::move(stream)) {
        ENFORCE(m_stream.is_valid(), "Cannot wrap an invalid SDL stream");
    }

    ~sdl_io_stream_impl() override {
        close();
    }

    size_t read(void* ptr, size_t size_bytes) override {
        return m_stream.read(ptr, size_bytes).value_or(0);
    }

    size_t write(const void* ptr, size_t size_bytes) override {
        return m_stream.write(ptr, size_bytes).value_or(0);
    }

    int64_t seek(int64_t offset, musac::seek_origin whence) override {
        sdlpp::io_seek_pos pos = sdlpp::io_seek_pos::set;
        switch (whence) {
            case musac::seek_origin::set: pos = sdlpp::io_seek_pos::set; break;
            case musac::seek_origin::cur: pos = sdlpp::io_seek_pos::current; break;
            case musac::seek_origin::end: pos = sdlpp::io_seek_pos::end; break;
            default: return -1;
        }
        return m_stream.seek(offset, pos).value_or(-1);
    }

    int64_t tell() override {
        return m_stream.tell().value_or(-1);
    }

    int64_t get_size() override {
        return m_stream.size().value_or(-1);
    }

    void close() override {
        m_stream = sdlpp::iostream{};
    }

    bool is_open() const override {
        return m_stream.is_valid();
    }

private:
    sdlpp::iostream m_stream;
};

std::unique_ptr<musac::io_stream> io_from_sdl(sdlpp::iostream stream) {
    if (!stream.is_valid()) {
        return nullptr;
    }
    return std::make_unique<sdl_io_stream_impl>(std::move(stream));
}

std::unique_ptr<musac::io_stream> io_from_file(const std::string& path) {
    auto file = sdlpp::open_file(path, sdlpp::file_mode::read_binary);
    if (!file) {
        return nullptr;
    }
    return io_from_sdl(std::move(*file));
}

} // namespace neutrino::audio_detail
