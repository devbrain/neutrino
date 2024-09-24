//
// Created by igor on 9/15/24.
//

#ifndef  RAW_LEVEL_MAP_HH
#define  RAW_LEVEL_MAP_HH

#include <vector>
#include <map>
#include <sdlpp/video/geometry.hh>

struct cell {
    static constexpr int EMPTY_TILE_NAME = -1;
    cell()
        : tile_name_id(EMPTY_TILE_NAME), flags(0) {
    }

    cell(int tile_name_id, unsigned int flags)
        : tile_name_id(tile_name_id),
          flags(flags) {
    }

    [[nodiscard]] bool is_empty() const {
        return tile_name_id == EMPTY_TILE_NAME;
    }

    [[nodiscard]] bool has_property(int prop) const {
        return props.find(prop) != props.end();
    }

    [[nodiscard]] int get_property(int prop, int default_value = -1) const {
        auto itr = props.find(prop);
        if (itr == props.end()) {
            return default_value;
        }
        return itr->second;
    }

    void set_property(int prop, int v) {
        props[prop] = v;
    }

	[[nodiscard]] bool is_interesting() const {
		return !is_empty() || flags != 0 || !props.empty();
	}

    int tile_name_id;
    unsigned int flags;
    std::map <int, int> props;
};

struct raw_level_map {
    raw_level_map(int level_num, const neutrino::sdl::area_type& dims);

    int level_num;
    neutrino::sdl::area_type dims;
    std::vector<int> backround; // array of tile ids
    std::vector<cell> cells;
    std::vector<uint8_t> raw_map;
};

#endif
