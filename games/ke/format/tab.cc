//
// TAB level parser.
//

#include <ke/format/tab.hh>
#include <bstream/span_reader.hh>

namespace rs {
    namespace {
        constexpr std::size_t LEVEL_SIZE = 586; // 2 (spawn_period) + 8 (spawn_seq) + 576 (cells)
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
            bstream::span_reader reader(data.subspan(li * LEVEL_SIZE, LEVEL_SIZE), bstream::endian::little);
            tab_level level;
            reader >> level.spawn_period >> level.spawn_seq;
            for (auto& cell : level.cells) {
                reader >> cell.attribute >> cell.tile;
            }
            levels.push_back(level);
        }

        return true;
    }
} // namespace ke
