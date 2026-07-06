//
// Created by igor on 05/07/2026.
//

#include "world/tmx/encoding.hh"

#include <cctype>
#include <cstddef>
#include <limits>
#include <string>

namespace neutrino::world_tmx {
    world_tile_cell decode_gid(std::uint32_t raw) {
        static constexpr std::uint32_t flipped_horizontally = 0x80000000u;
        static constexpr std::uint32_t flipped_vertically = 0x40000000u;
        static constexpr std::uint32_t flipped_diagonally = 0x20000000u;
        static constexpr std::uint32_t rotated_hexagonal_120 = 0x10000000u;

        sprite_flip flip = sprite_flip::none;
        if ((raw & flipped_horizontally) != 0) {
            flip |= sprite_flip::horizontal;
        }
        if ((raw & flipped_vertically) != 0) {
            flip |= sprite_flip::vertical;
        }
        if ((raw & flipped_diagonally) != 0) {
            flip |= sprite_flip::diagonal;
        }

        raw &= ~(flipped_horizontally | flipped_vertically | flipped_diagonally | rotated_hexagonal_120);
        return {raw, flip};
    }

    std::vector <std::uint32_t> parse_csv_values(std::string_view data) {
        std::vector <std::uint32_t> result;
        std::string token;
        auto flush = [&]() {
            auto clean = trim(token);
            token.clear();
            if (!clean.empty()) {
                result.push_back(parse_integral <std::uint32_t>(clean, "csv-data"));
            }
        };

        for (const auto ch : data) {
            if (ch == ',' || std::isspace(static_cast <unsigned char>(ch)) != 0) {
                flush();
            } else {
                token.push_back(ch);
            }
        }
        flush();
        return result;
    }

    namespace {
        [[nodiscard]] int base64_value(char ch) noexcept {
            if (ch >= 'A' && ch <= 'Z') {
                return ch - 'A';
            }
            if (ch >= 'a' && ch <= 'z') {
                return ch - 'a' + 26;
            }
            if (ch >= '0' && ch <= '9') {
                return ch - '0' + 52;
            }
            if (ch == '+') {
                return 62;
            }
            if (ch == '/') {
                return 63;
            }
            return -1;
        }

        [[nodiscard]] byte_buffer decode_base64(std::string_view data) {
            byte_buffer output;
            int value = 0;
            int bits = -8;
            for (const auto ch : data) {
                if (std::isspace(static_cast <unsigned char>(ch)) != 0) {
                    continue;
                }
                if (ch == '=') {
                    break;
                }
                const auto decoded = base64_value(ch);
                if (decoded < 0) {
                    fail("invalid base64 character in tile data");
                }
                value = (value << 6) + decoded;
                bits += 6;
                if (bits >= 0) {
                    output.push_back(static_cast <std::uint8_t>((value >> bits) & 0xff));
                    bits -= 8;
                }
            }
            return output;
        }

        void validate_cell_count(
            std::size_t actual,
            std::optional <std::size_t> expected,
            std::string_view what) {
            if (expected && actual != *expected) {
                fail(
                    make_string(what) + " contains " + std::to_string(actual) +
                    " cells, expected " + std::to_string(*expected));
            }
        }

        [[nodiscard]] std::optional <std::size_t> expected_byte_count(
            std::optional <std::size_t> expected_cells) {
            if (!expected_cells) {
                return std::nullopt;
            }
            if (*expected_cells > std::numeric_limits <std::size_t>::max() / 4) {
                fail("expected tile data size is too large");
            }
            return *expected_cells * 4;
        }

        [[nodiscard]] std::vector <world_tile_cell> cells_from_words(
            const byte_buffer& data,
            std::optional <std::size_t> expected_cells) {
            if (data.size() % 4 != 0) {
                fail("binary tile data size is not divisible by 4");
            }
            validate_cell_count(data.size() / 4, expected_cells, "binary tile data");
            std::vector <world_tile_cell> result;
            result.reserve(data.size() / 4);
            for (std::size_t i = 0; i < data.size(); i += 4) {
                const auto gid =
                    static_cast <std::uint32_t>(data[i]) |
                    (static_cast <std::uint32_t>(data[i + 1]) << 8u) |
                    (static_cast <std::uint32_t>(data[i + 2]) << 16u) |
                    (static_cast <std::uint32_t>(data[i + 3]) << 24u);
                result.push_back(decode_gid(gid));
            }
            return result;
        }
    }

    std::vector <world_tile_cell> parse_encoded_cells(
        const std::string& encoding,
        const std::string& compression,
        std::string_view data,
        std::optional <std::size_t> expected_cells) {
        if (encoding.empty() || encoding == "csv") {
            if (!compression.empty()) {
                fail("compression is not valid for CSV tile data");
            }
            auto gids = parse_csv_values(data);
            validate_cell_count(gids.size(), expected_cells, "CSV tile data");
            std::vector <world_tile_cell> result;
            result.reserve(gids.size());
            for (const auto gid : gids) {
                result.push_back(decode_gid(gid));
            }
            return result;
        }
        if (encoding != "base64") {
            fail("unsupported tile data encoding <" + encoding + ">");
        }
        auto bytes = decode_base64(data);
        bytes = decompressor::decompress(bytes, compression, expected_byte_count(expected_cells));
        return cells_from_words(bytes, expected_cells);
    }

    byte_buffer parse_encoded_bytes(
        const std::string& encoding,
        const std::string& compression,
        std::string_view data) {
        if (encoding != "base64") {
            fail("embedded image data must use base64 encoding");
        }
        auto bytes = decode_base64(data);
        return decompressor::decompress(bytes, compression);
    }
}
