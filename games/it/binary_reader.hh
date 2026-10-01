//
// Created by igor on 10/06/2026.
//

#pragma once

#include <istream>
#include <streambuf>
#include <vector>
#include <type_traits>
#include <array>
#include <span>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <cstddef>

// ============================================================================
// Exceptions
// ============================================================================
class binary_reader_error : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
};

class binary_reader_eof_error : public binary_reader_error {
    public:
        using binary_reader_error::binary_reader_error;
};

// ============================================================================
// trivially_copyable Concept
// ============================================================================
template<typename T>
concept trivially_copyable = std::is_trivially_copyable_v <T>;

class binary_reader {
    public:
        using error = binary_reader_error;
        using eof_error = binary_reader_eof_error;

        explicit binary_reader(std::istream& is)
            : is_(is) {
        }

        // Direct raw bytes read
        void read(void* dst, std::size_t size) {
            read_bytes(static_cast<char*>(dst), size);
        }

        // Typed read helper: auto val = reader.read<uint32_t>();
        template<trivially_copyable T>
        T read() {
            T val{};
            *this >> val;
            return val;
        }

        // 1. Single element overload: Constrain to trivially copyable types (PODs, integrals, floats)
        template<trivially_copyable T>
        binary_reader& operator>>(T& val) {
            read_bytes(reinterpret_cast <char*>(&val), sizeof(T));
            return *this;
        }

        // 2. Vector overload for trivially copyable types: performs a bulk read
        template<trivially_copyable T>
        binary_reader& operator>>(std::vector <T>& vec) {
            if (!vec.empty()) {
                read_bytes(reinterpret_cast <char*>(vec.data()), vec.size() * sizeof(T));
            }
            return *this;
        }

        // 3. Vector overload for non-trivially copyable types: loops and deserializes elements individually
        template<typename T> requires (!std::is_trivially_copyable_v <T>)
        binary_reader& operator>>(std::vector <T>& vec) {
            for (auto& elem : vec) {
                *this >> elem; // Recursively delegates to operator>>
            }
            return *this;
        }

        // 4. Overload for C-style arrays (e.g., uint32_t reserved[5])
        template<typename T, std::size_t N> requires std::is_trivially_copyable_v <T>
        binary_reader& operator>>(T (& val)[N]) {
            read_bytes(reinterpret_cast <char*>(val), N * sizeof(T));
            return *this;
        }

        // 5. Overload for C-style arrays (e.g., uint32_t reserved[5])
        template<typename T, std::size_t N> requires (!std::is_trivially_copyable_v <T>)
        binary_reader& operator>>(T (& val)[N]) {
            for (std::size_t i = 0; i < N; i++) {
                *this >> val[i]; // Recursively delegates to operator>>
            }
            return *this;
        }

        // 6. std::array overload for trivially copyable types: performs a bulk read
        template<typename T, std::size_t N> requires std::is_trivially_copyable_v <T>
        binary_reader& operator>>(std::array <T, N>& arr) {
            if (!arr.empty()) {
                read_bytes(reinterpret_cast <char*>(arr.data()), N * sizeof(T));
            }
            return *this;
        }

        // 7. std::array overload for non-trivially copyable types: loops and deserializes elements individually
        template<typename T, std::size_t N> requires (!std::is_trivially_copyable_v <T>)
        binary_reader& operator>>(std::array <T, N>& arr) {
            for (auto& elem : arr) {
                *this >> elem; // Recursively delegates to operator>>
            }
            return *this;
        }

        // Seeking and positioning
        binary_reader& seek(std::streamoff off, std::ios_base::seekdir dir = std::ios_base::beg) {
            if (is_.eof()) {
                is_.clear();
            }
            if (!is_.seekg(off, dir)) {
                throw binary_reader_error("binary_reader: seek failed");
            }
            return *this;
        }

        binary_reader& seek(std::streampos pos) {
            if (is_.eof()) {
                is_.clear();
            }
            if (!is_.seekg(pos)) {
                throw binary_reader_error("binary_reader: seek failed");
            }
            return *this;
        }

        [[nodiscard]] std::streampos tell() const {
            auto pos = is_.tellg();
            if (pos == std::streampos(-1) || is_.fail()) {
                throw binary_reader_error("binary_reader: tell failed");
            }
            return pos;
        }

        // Expose stream state queries
        explicit operator bool() const { return static_cast <bool>(is_); }
        bool operator!() const { return !is_; }
        [[nodiscard]] bool eof() const { return is_.eof(); }
        [[nodiscard]] bool fail() const { return is_.fail(); }
        [[nodiscard]] bool bad() const { return is_.bad(); }
        void clear() { is_.clear(); }
        [[nodiscard]] std::istream& stream() const { return is_; }

    private:
        void read_bytes(char* dst, std::size_t size) const {
            if (size == 0) {
                return;
            }
            if (!is_) {
                if (is_.eof()) {
                    throw binary_reader_eof_error("binary_reader: stream is already at EOF before read");
                }
                throw binary_reader_error("binary_reader: stream is in an error state before read");
            }
            is_.read(dst, static_cast <std::streamsize>(size));
            if (!is_) {
                const auto bytes_read = is_.gcount();
                if (is_.eof()) {
                    throw binary_reader_eof_error(
                        "binary_reader: unexpected end of stream (requested " +
                        std::to_string(size) + " bytes, read " +
                        std::to_string(bytes_read) + " bytes)");
                }
                throw binary_reader_error(
                    "binary_reader: read failed (requested " +
                    std::to_string(size) + " bytes, read " +
                    std::to_string(bytes_read) + " bytes)");
            }
        }

        std::istream& is_;
};

// ============================================================================
// Memory Stream Buffer and span_reader
// ============================================================================
class membuf : public std::streambuf {
    public:
        membuf(const void* base, std::size_t size) {
            char* p = const_cast<char*>(static_cast<const char*>(base));
            setg(p, p, p + size);
        }

    protected:
        pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in) override {
            if (!(which & std::ios_base::in)) return pos_type(off_type(-1));
            char* new_gptr = nullptr;
            if (dir == std::ios_base::beg) {
                new_gptr = eback() + off;
            } else if (dir == std::ios_base::cur) {
                new_gptr = gptr() + off;
            } else if (dir == std::ios_base::end) {
                new_gptr = egptr() + off;
            } else {
                return pos_type(off_type(-1));
            }
            if (new_gptr < eback() || new_gptr > egptr()) {
                return pos_type(off_type(-1));
            }
            setg(eback(), new_gptr, egptr());
            return pos_type(new_gptr - eback());
        }

        pos_type seekpos(pos_type sp, std::ios_base::openmode which = std::ios_base::in) override {
            return seekoff(off_type(sp), std::ios_base::beg, which);
        }
};

namespace detail {
struct span_stream_holder {
    membuf buf;
    std::istream stream;
    span_stream_holder(const void* data, std::size_t size)
        : buf(data, size), stream(&buf) {}
};
} // namespace detail

class span_reader : private detail::span_stream_holder, public binary_reader {
    public:
        explicit span_reader(std::span<const uint8_t> s)
            : span_reader(s.data(), s.size_bytes()) {
        }

        explicit span_reader(std::span<const char> s)
            : span_reader(s.data(), s.size_bytes()) {
        }

        template<typename Container> requires (!std::is_pointer_v<std::decay_t<Container>> && requires(const Container& c) { { std::data(c) }; { std::size(c) }; })
        explicit span_reader(const Container& c)
            : span_reader(std::data(c), std::size(c) * sizeof(*std::data(c))) {
        }

        explicit span_reader(const void* data, std::size_t size)
            : detail::span_stream_holder(data, size),
              binary_reader(detail::span_stream_holder::stream) {
        }
};
