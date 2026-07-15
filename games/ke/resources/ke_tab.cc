//
// TAB level parser.
//

#include "resources/ke_tab.hh"

namespace rs {
    namespace {
        constexpr std::size_t LEVEL_SIZE = 586; // 2 (spawn_period) + 8 (spawn_seq) + 576 (cells)
        constexpr std::size_t CELLS_OFFSET = 10;
    } // namespace

    bool tab_levels::parse(std::span <const std::uint8_t> data, std::string& error) {
        levels.clear();

        const std::size_t count = data.size() / LEVEL_SIZE;
        if (count == 0) {
            error = "TAB: resource smaller than one level (586 bytes)";
            return false;
        }

        levels.reserve(count);
        for (std::size_t li = 0; li < count; ++li) {
            const std::uint8_t* p = data.data() + li * LEVEL_SIZE;

            tab_level level;
            level.spawn_period = static_cast <std::uint16_t>(p[0] | (p[1] << 8));
            for (int i = 0; i < 8; ++i) level.spawn_seq[i] = p[2 + i];

            const std::uint8_t* cells = p + CELLS_OFFSET;
            for (int y = 0; y < tab_level::rows; ++y) {
                for (int x = 0; x < tab_level::cols; ++x) {
                    const std::size_t idx = static_cast <std::size_t>(y) * tab_level::cols + x;
                    level.cells[idx].attribute = cells[idx * 2 + 0];
                    level.cells[idx].tile = cells[idx * 2 + 1];
                }
            }
            levels.push_back(level);
        }

        return true;
    }
} // namespace ke
