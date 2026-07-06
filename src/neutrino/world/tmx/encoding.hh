//
// Created by igor on 05/07/2026.
//

#pragma once

#include "world/tmx/decompressor.hh"
#include "world/tmx/node.hh"

#include <string>
#include <string_view>
#include <optional>
#include <vector>

namespace neutrino::world_tmx {
    [[nodiscard]] world_tile_cell decode_gid(std::uint32_t raw);
    [[nodiscard]] std::vector <std::uint32_t> parse_csv_values(std::string_view data);
    [[nodiscard]] std::vector <world_tile_cell> parse_encoded_cells(
        const std::string& encoding,
        const std::string& compression,
        std::string_view data,
        std::optional <std::size_t> expected_cells = std::nullopt);
    [[nodiscard]] byte_buffer parse_encoded_bytes(
        const std::string& encoding,
        const std::string& compression,
        std::string_view data);
}
