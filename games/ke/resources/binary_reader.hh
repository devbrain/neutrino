//
// Created by igor on 10/06/2026.
//

#pragma once

#include <istream>
#include <vector>
#include <type_traits>
#include <array>
#include <bit>
#include <algorithm>

// ============================================================================
// endian_scalar Concept
// ============================================================================
// Types the reader can byte-swap on its own: integrals, floating point, and
// enums. Structs are DELIBERATELY excluded -- a struct must provide its own
// operator>>(binary_reader&, T&). A raw `sizeof` read would neither byte-swap
// individual fields nor account for padding, silently defeating the reader's
// whole purpose; excluding structs turns that mistake into a compile error.
template<typename T>
concept endian_scalar = std::is_arithmetic_v <T> || std::is_enum_v <T>;

class binary_reader {
    public:
        explicit binary_reader(std::istream& is, std::endian endian = std::endian::native)
            : is_(is), endian_(endian) {
        }

        // 1. Single scalar (integral / floating point / enum).
        template<endian_scalar T>
        binary_reader& operator>>(T& val) {
            is_.read(reinterpret_cast <char*>(&val), sizeof(T));
            swap_if_needed(val);
            return *this;
        }

        // 2. Vector of scalars: one bulk read, then swap each element if needed.
        template<endian_scalar T>
        binary_reader& operator>>(std::vector <T>& vec) {
            if (!vec.empty()) {
                is_.read(reinterpret_cast <char*>(vec.data()), vec.size() * sizeof(T));
                swap_each(vec);
            }
            return *this;
        }

        // 3. Vector of non-scalars: deserialize elements individually (each needs its own operator>>).
        template<typename T> requires (!endian_scalar <T>)
        binary_reader& operator>>(std::vector <T>& vec) {
            for (auto& elem : vec) {
                *this >> elem; // recurses; requires operator>>(binary_reader&, T&)
            }
            return *this;
        }

        // 4. C-array of scalars (e.g. uint32_t reserved[5]).
        template<endian_scalar T, std::size_t N>
        binary_reader& operator>>(T (& val)[N]) {
            is_.read(reinterpret_cast <char*>(val), N * sizeof(T));
            swap_each(val);
            return *this;
        }

        // 5. C-array of non-scalars.
        template<typename T, std::size_t N> requires (!endian_scalar <T>)
        binary_reader& operator>>(T (& val)[N]) {
            for (std::size_t i = 0; i < N; ++i) {
                *this >> val[i];
            }
            return *this;
        }

        // 6. std::array of scalars: one bulk read, then swap each element if needed.
        template<endian_scalar T, std::size_t N>
        binary_reader& operator>>(std::array <T, N>& arr) {
            if (!arr.empty()) {
                is_.read(reinterpret_cast <char*>(arr.data()), arr.size() * sizeof(T));
                swap_each(arr);
            }
            return *this;
        }

        // 7. std::array of non-scalars.
        template<typename T, std::size_t N> requires (!endian_scalar <T>)
        binary_reader& operator>>(std::array <T, N>& arr) {
            for (auto& elem : arr) {
                *this >> elem;
            }
            return *this;
        }

        // Expose stream state queries
        explicit operator bool() const { return static_cast <bool>(is_); }
        bool operator!() const { return !is_; }
        [[nodiscard]] std::istream& stream() const { return is_; }

        // Endianness configuration
        void set_endian(std::endian endian) { endian_ = endian; }
        [[nodiscard]] std::endian endian() const { return endian_; }

    private:
        template<endian_scalar T>
        static void swap_bytes(T& val) {
            if constexpr (sizeof(T) > 1) {
                auto* ptr = reinterpret_cast <char*>(&val);
                std::reverse(ptr, ptr + sizeof(T));
            }
        }

        template<endian_scalar T>
        void swap_if_needed(T& val) {
            if (endian_ != std::endian::native) {
                swap_bytes(val);
            }
        }

        // Byte-swap every element of a scalar range in place, if the stream's
        // endianness differs from the host.
        template<typename Range>
        void swap_each(Range& range) {
            if (endian_ != std::endian::native) {
                for (auto& val : range) {
                    swap_bytes(val);
                }
            }
        }

        std::istream& is_;
        std::endian endian_;
};
