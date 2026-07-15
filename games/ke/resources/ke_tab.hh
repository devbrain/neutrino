//
// TAB level parser. See docs/tab.md for the format specification.
//

#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace rs {
    // One grid cell: a collision/type attribute plus a graphical tile index.
    struct tab_cell {
        std::uint8_t attribute = 0;
        std::uint8_t tile = 0;
    };

    // A single level: spawn timing/sequence plus the 16x18 cell grid.
    struct tab_level {
        static constexpr int cols = 18;
        static constexpr int rows = 16;

        std::uint16_t spawn_period = 0;
        std::array <std::uint8_t, 8> spawn_seq{};
        std::array <tab_cell, static_cast <std::size_t>(cols) * rows> cells{}; // row-major

        [[nodiscard]] const tab_cell& at(int x, int y) const {
            return cells[static_cast <std::size_t>(y) * cols + x];
        }
    };

    // A parsed TAB resource: a packed array of 586-byte levels.
    struct tab_levels {
        std::vector <tab_level> levels;

        // Parse a TAB resource. Returns false and sets `error` if no complete level
        // is present; trailing bytes that don't form a full 586-byte level are ignored.
        bool parse(std::span <const std::uint8_t> data, std::string& error);
    };
} // namespace ke
