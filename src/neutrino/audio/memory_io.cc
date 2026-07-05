//
// Created by igor on 02/07/2026.
//

#include "memory_io.hh"

namespace neutrino::audio_detail {

namespace {
    // Delegates to musac::io_from_memory while co-owning the buffer, since
    // io_from_memory itself requires the memory to outlive the stream.
    class shared_buffer_io final : public musac::io_stream {
    public:
        explicit shared_buffer_io(std::shared_ptr<const std::vector<uint8_t>> data)
            : m_data(std::move(data)),
              m_inner(musac::io_from_memory(m_data->data(), m_data->size())) {
        }

        size_t read(void* ptr, size_t size_bytes) override {
            return m_inner ? m_inner->read(ptr, size_bytes) : 0;
        }

        size_t write(const void* ptr, size_t size_bytes) override {
            return m_inner ? m_inner->write(ptr, size_bytes) : 0;
        }

        int64_t seek(int64_t offset, musac::seek_origin whence) override {
            return m_inner ? m_inner->seek(offset, whence) : -1;
        }

        int64_t tell() override {
            return m_inner ? m_inner->tell() : -1;
        }

        int64_t get_size() override {
            return m_inner ? m_inner->get_size() : -1;
        }

        void close() override {
            if (m_inner) {
                m_inner->close();
            }
        }

        bool is_open() const override {
            return m_inner && m_inner->is_open();
        }

    private:
        std::shared_ptr<const std::vector<uint8_t>> m_data;
        std::unique_ptr<musac::io_stream> m_inner;
    };
}

std::shared_ptr<const std::vector<uint8_t>> read_all(std::istream& is) {
    auto buf = std::make_shared<std::vector<uint8_t>>();
    char chunk[4096];
    while (true) {
        is.read(chunk, sizeof(chunk));
        const auto got = is.gcount();
        if (got > 0) {
            buf->insert(buf->end(), chunk, chunk + got);
        }
        if (!is) {
            break;
        }
    }
    return buf;
}

std::unique_ptr<musac::io_stream> io_from_buffer(std::shared_ptr<const std::vector<uint8_t>> data) {
    if (!data || data->empty()) {
        return nullptr;
    }
    return std::make_unique<shared_buffer_io>(std::move(data));
}

} // namespace neutrino::audio_detail
