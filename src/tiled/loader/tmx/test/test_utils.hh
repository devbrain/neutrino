//
// Created by igor on 24/07/2021.
//

#ifndef NEUTRINO_TEST_UTILS_HH
#define NEUTRINO_TEST_UTILS_HH


#include "tiled/loader/tmx/reader.hh"
#include "tiled/loader/tmx/map.hh"

namespace neutrino::tiled::tmx::test {

    inline std::string null_resolver(const std::string&) {return "";}

    map load_map(const unsigned char* data, std::size_t length, path_resolver_t resolver = null_resolver);

    bool check_properties(const component& obj, const std::map<std::string, property_t>& props);
    bool test_tiles(const tile_layer& tl, const std::vector<int>& expected);
    bool eq_cells (const std::vector<cell>& a, const std::vector<cell>& b);
}

#endif

