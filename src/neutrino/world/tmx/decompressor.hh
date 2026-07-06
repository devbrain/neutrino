//
// Created by igor on 05/07/2026.
//

#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace neutrino::world_tmx {
    using byte_buffer = std::vector <std::uint8_t>;

    class decompressor {
        public:
            [[nodiscard]] static byte_buffer decompress(
                const byte_buffer& input,
                std::string_view compression,
                std::optional <std::size_t> max_output_size = std::nullopt);

        private:
            [[nodiscard]] static byte_buffer zlib(
                const byte_buffer& input,
                std::optional <std::size_t> max_output_size);
            [[nodiscard]] static byte_buffer gzip(
                const byte_buffer& input,
                std::optional <std::size_t> max_output_size);
            [[nodiscard]] static byte_buffer zstd(
                const byte_buffer& input,
                std::optional <std::size_t> max_output_size);
    };
}
