//
// Created by igor on 24/07/2021.
//

#include "test_utils.hh"
#include <sstream>
#include <neutrino/utils/io/memory_stream_buf.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::tiled::tmx::test {
    map load_map(const unsigned char* data, std::size_t length, path_resolver_t resolver) {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_buffer((char*)data, length);
        if (!result)
        {
            RAISE_EX("Failed to load tmx :", result.description());
        }
        auto root = doc.child("map");
        if (!root)
        {
            RAISE_EX ("entry node <map> is missing");
        }
        return map::parse(xml_node(root), resolver);
    }
    // ---------------------------------------------------------------------------------
    bool check_properties(const component& obj, const std::map<std::string, property_t>& props) {
        for (const auto& [name, val] : props) {
            if (auto pprop = obj.get(name); pprop.has_value()) {
                if (*pprop != val) {
                    return false;
                }
            } else {
                return false;
            }
        }
        return true;
    }
    // ---------------------------------------------------------------------------------
    bool test_tiles(const tile_layer& tl, const std::vector<int>& expected) {
        int k = 0;
        for (const auto c : tl)
        {
            if(k >= expected.size()) {
                return false;
            }
            if (c.gid() != expected[k++]) {
                return false;
            }
        }
        return (k == expected.size());
    }
    bool eq_cells (const std::vector<cell>& a, const std::vector<cell>& b) {
        if (a.size() != b.size()) {
            return false;
        }
        for (std::size_t i=0; i<a.size(); i++) {
            if (a[i].hor_flipped() != b[i].hor_flipped()) {
                return false;
            }
            if (a[i].vert_flipped() != b[i].vert_flipped()) {
                return false;
            }
            if (a[i].diag_flipped() != b[i].diag_flipped()) {
                return false;
            }
            if (a[i].gid() != b[i].gid()) {
                return false;
            }
        }
        return true;
    }
}