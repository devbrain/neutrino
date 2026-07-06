//
// Created by igor on 05/07/2026.
//

#include "world/tmx/decompressor.hh"

#include "world/tmx/node.hh"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>

#if defined(NEUTRINO_TMX_ENABLE_ZSTD)
#include <zstd.h>
#endif

#define SINFL_IMPLEMENTATION
#define sinflate neutrino_tmx_sinflate
#define zsinflate neutrino_tmx_zsinflate
#include "world/tmx/sinfl.h"
#undef zsinflate
#undef sinflate
#undef SINFL_IMPLEMENTATION

namespace neutrino::world_tmx {
    namespace {
        [[nodiscard]] int to_int_size(std::size_t size, std::string_view what) {
            if (size > static_cast <std::size_t>(std::numeric_limits <int>::max())) {
                fail(make_string(what) + " is too large for sinfl");
            }
            return static_cast <int>(size);
        }

        [[nodiscard]] byte_buffer run_sinfl(
            int (*inflate)(void*, int, const void*, int),
            const std::uint8_t* data,
            std::size_t size,
            std::string_view format,
            std::optional <std::size_t> max_output_size) {
            const auto input_size = to_int_size(size, std::string{"compressed "} + make_string(format) + " data");
            if (max_output_size && *max_output_size > static_cast <std::size_t>(std::numeric_limits <int>::max())) {
                fail("decompressed " + make_string(format) + " data is too large for sinfl");
            }

            byte_buffer padded_input(size + 8);
            std::copy_n(data, size, padded_input.data());

            auto capacity = std::max <std::size_t>(size * 4, 32 * 1024);
            if (max_output_size) {
                capacity = std::min(capacity, *max_output_size);
            }

            while (capacity <= static_cast <std::size_t>(std::numeric_limits <int>::max())) {
                byte_buffer output(capacity);
                const auto produced = inflate(
                    output.data(),
                    to_int_size(output.size(), "decompressed buffer"),
                    padded_input.data(),
                    input_size);
                if (produced >= 0) {
                    output.resize(static_cast <std::size_t>(produced));
                    return output;
                }
                if (produced != -2) {
                    fail("failed to decompress " + make_string(format) + " data");
                }

                if (max_output_size && capacity >= *max_output_size) {
                    fail("decompressed " + make_string(format) + " data exceeds expected size");
                }
                if (capacity > static_cast <std::size_t>(std::numeric_limits <int>::max()) / 2) {
                    break;
                }
                capacity *= 2;
                if (max_output_size) {
                    capacity = std::min(capacity, *max_output_size);
                }
            }

            fail("decompressed " + make_string(format) + " data is too large");
        }

        [[nodiscard]] std::uint16_t read_le16(const byte_buffer& data, std::size_t offset) {
            if (offset + 2 > data.size()) {
                fail("truncated gzip header");
            }
            return static_cast <std::uint16_t>(
                static_cast <std::uint16_t>(data[offset]) |
                (static_cast <std::uint16_t>(data[offset + 1]) << 8u));
        }

        [[nodiscard]] std::uint32_t read_le32(const byte_buffer& data, std::size_t offset) {
            if (offset + 4 > data.size()) {
                fail("truncated gzip trailer");
            }
            return
                static_cast <std::uint32_t>(data[offset]) |
                (static_cast <std::uint32_t>(data[offset + 1]) << 8u) |
                (static_cast <std::uint32_t>(data[offset + 2]) << 16u) |
                (static_cast <std::uint32_t>(data[offset + 3]) << 24u);
        }

        void skip_zero_terminated_field(const byte_buffer& data, std::size_t& offset) {
            while (offset < data.size() && data[offset] != 0) {
                ++offset;
            }
            if (offset >= data.size()) {
                fail("unterminated gzip header field");
            }
            ++offset;
        }

        [[nodiscard]] std::uint32_t crc32(const byte_buffer& data) {
            auto crc = 0xffffffffu;
            for (const auto byte : data) {
                crc ^= byte;
                for (int bit = 0; bit < 8; ++bit) {
                    const auto mask = 0u - (crc & 1u);
                    crc = (crc >> 1u) ^ (0xedb88320u & mask);
                }
            }
            return crc ^ 0xffffffffu;
        }
    }

    byte_buffer decompressor::decompress(
        const byte_buffer& input,
        std::string_view compression,
        std::optional <std::size_t> max_output_size) {
        if (compression.empty()) {
            if (max_output_size && input.size() > *max_output_size) {
                fail("uncompressed data exceeds expected size");
            }
            return input;
        }
        if (compression == "zlib") {
            return zlib(input, max_output_size);
        }
        if (compression == "gzip") {
            return gzip(input, max_output_size);
        }
        if (compression == "zstd") {
            return zstd(input, max_output_size);
        }
        fail("unsupported compression <" + make_string(compression) + ">");
    }

    byte_buffer decompressor::zlib(const byte_buffer& input, std::optional <std::size_t> max_output_size) {
        return run_sinfl(
            ::neutrino_tmx_zsinflate,
            input.data(),
            input.size(),
            "zlib",
            max_output_size);
    }

    byte_buffer decompressor::gzip(const byte_buffer& input, std::optional <std::size_t> max_output_size) {
        if (input.size() < 18) {
            fail("gzip data is too small");
        }
        if (input[0] != 0x1f || input[1] != 0x8b) {
            fail("gzip data has an invalid magic header");
        }
        if (input[2] != 8) {
            fail("gzip data uses an unsupported compression method");
        }
        const auto flags = input[3];
        if ((flags & 0xe0u) != 0) {
            fail("gzip data has reserved flags set");
        }

        std::size_t offset = 10;
        if ((flags & 0x04u) != 0) {
            const auto extra_length = read_le16(input, offset);
            offset += 2 + extra_length;
            if (offset > input.size()) {
                fail("truncated gzip extra field");
            }
        }
        if ((flags & 0x08u) != 0) {
            skip_zero_terminated_field(input, offset);
        }
        if ((flags & 0x10u) != 0) {
            skip_zero_terminated_field(input, offset);
        }
        if ((flags & 0x02u) != 0) {
            offset += 2;
        }

        if (offset + 8 > input.size()) {
            fail("gzip data has no compressed payload");
        }

        const auto compressed_size = input.size() - offset - 8;
        auto output = run_sinfl(
            ::neutrino_tmx_sinflate,
            input.data() + offset,
            compressed_size,
            "gzip",
            max_output_size);

        const auto expected_crc = read_le32(input, input.size() - 8);
        const auto expected_size = read_le32(input, input.size() - 4);
        if (crc32(output) != expected_crc) {
            fail("gzip data CRC check failed");
        }
        if ((output.size() & 0xffffffffu) != expected_size) {
            fail("gzip data size check failed");
        }
        return output;
    }

    byte_buffer decompressor::zstd(const byte_buffer& input, std::optional <std::size_t> max_output_size) {
#if defined(NEUTRINO_TMX_ENABLE_ZSTD)
        const auto frame_size = ZSTD_getFrameContentSize(input.data(), input.size());
        if (frame_size == ZSTD_CONTENTSIZE_ERROR) {
            fail("zstd data is not a valid frame");
        }
        if (frame_size != ZSTD_CONTENTSIZE_UNKNOWN) {
            if (frame_size > static_cast <unsigned long long>(std::numeric_limits <std::size_t>::max())) {
                fail("zstd frame is too large for this platform");
            }
            if (max_output_size && frame_size > *max_output_size) {
                fail("zstd frame exceeds expected decompressed size");
            }

            byte_buffer output(static_cast <std::size_t>(frame_size));
            const auto written = ZSTD_decompress(output.data(), output.size(), input.data(), input.size());
            if (ZSTD_isError(written) != 0) {
                fail(std::string("failed to decompress zstd tile data: ") + ZSTD_getErrorName(written));
            }
            output.resize(written);
            return output;
        }

        struct stream_guard {
            ZSTD_DStream* stream{};

            ~stream_guard() {
                ZSTD_freeDStream(stream);
            }
        } guard{ZSTD_createDStream()};

        if (guard.stream == nullptr) {
            fail("failed to create zstd decompression stream");
        }

        auto status = ZSTD_initDStream(guard.stream);
        if (ZSTD_isError(status) != 0) {
            fail(std::string("failed to initialize zstd decompression stream: ") + ZSTD_getErrorName(status));
        }

        const auto chunk_size = std::max <std::size_t>(ZSTD_DStreamOutSize(), 32 * 1024);
        ZSTD_inBuffer source{input.data(), input.size(), 0};
        byte_buffer output;

        while (source.pos < source.size) {
            const auto previous_input_pos = source.pos;
            const auto previous_output_size = output.size();
            auto available = chunk_size;
            if (max_output_size) {
                if (previous_output_size >= *max_output_size) {
                    fail("zstd data exceeds expected decompressed size");
                }
                available = std::min(available, *max_output_size - previous_output_size);
            }
            output.resize(previous_output_size + available);

            ZSTD_outBuffer target{output.data() + previous_output_size, available, 0};
            status = ZSTD_decompressStream(guard.stream, &target, &source);
            if (ZSTD_isError(status) != 0) {
                fail(std::string("failed to decompress zstd tile data: ") + ZSTD_getErrorName(status));
            }

            output.resize(previous_output_size + target.pos);
            if (source.pos == previous_input_pos && output.size() == previous_output_size) {
                fail("zstd decompression made no progress");
            }
        }

        if (status != 0) {
            fail("zstd data ended before the frame was complete");
        }
        return output;
#else
        static_cast <void>(input);
        fail("zstd-compressed TMX data is disabled in this build");
#endif
    }
}
