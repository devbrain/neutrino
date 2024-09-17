//
// Created by igor on 7/12/24.
//

#ifndef  CRYSTAL_CAVES_HH
#define  CRYSTAL_CAVES_HH

#include <istream>
#include <map>
#include <assets/assets.hh>

#include "cc_tile_ids.hh"

class exe_map_props {
    public:
        exe_map_props(std::size_t offset_, int columns_, int default_rows_, const std::map <int, int>& exceptions)
            : offset(offset_), default_rows(default_rows_), columns(columns_), lvl_rows(exceptions) {
        }

        exe_map_props(int columns_, int default_rows_)
            : offset(0), default_rows(default_rows_), columns(columns_) {
        }

        [[nodiscard]] std::streamoff get_offset() const {
            return static_cast <std::streamoff>(offset);
        }

        [[nodiscard]] int get_columns() const {
            return columns;
        }

        [[nodiscard]] int get_rows(int level) const {
            auto itr = lvl_rows.find(level);
            if (itr == lvl_rows.end()) {
                return default_rows;
            }
            return itr->second;
        }

    private:
        std::size_t offset;
        int default_rows;
        int columns;
        std::map <int, int> lvl_rows;
};

extern exe_map_props cc1;
extern exe_map_props cc3;


neutrino::assets::tileset load_tileset_cc(std::istream& is);
neutrino::assets::tileset load_mini_tileset_cc(std::istream& is);

struct cc_level_description {
    cc_level_description(int level,
                         const neutrino::sdl::area_type& level_dims)
        : level(level),
          level_dims(level_dims) {
    }

    int level;
    neutrino::sdl::area_type level_dims;
    std::vector <int> tiles;
};

std::vector <cc_level_description> extract_raw_levels_cc(std::istream& is, const exe_map_props& props);

#endif
