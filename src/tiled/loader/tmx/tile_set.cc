//
// Created by igor on 20/07/2021.
//

#include "tile_set.hh"
#include "xml_reader.hh"
#include <neutrino/utils/exception.hh>

namespace neutrino::tiled::tmx {
    const tile* tile_set::get_tile(unsigned id) const noexcept {
        for (auto& tile : m_tiles) {
            if (tile.id() == id) {
                return &tile;
            }
        }
        return nullptr;
    }

    tile_set::grid tile_set::grid::parse(const reader& elt) {
        try {
            auto ortho = elt.get_string_attribute("orientation") == "orthogonal";
            auto w = elt.get_uint_attribute("width");
            auto h = elt.get_uint_attribute("height");
            return {ortho, w, h};
        } catch (exception& e) {
            RAISE_EX_WITH_CAUSE(std::move(e), "Failed to parse grid section");
        }
    }

    math::rect tile_set::get_coords(unsigned id, math::dimension_t size) const noexcept {
        unsigned width = (size[0] - 2 * m_margin + m_spacing) / (m_tilewidth + m_spacing); // number of tiles
        unsigned height = (size[1] - 2 * m_margin + m_spacing) / (m_tileheight + m_spacing); // number of tiles

        unsigned tu = id % width;
        unsigned tv = id / width;
        ENFORCE(tv < height);

        unsigned du = m_margin + tu * m_spacing + m_x;
        unsigned dv = m_margin + tv * m_spacing + m_y;
        ENFORCE((int)((tu + 1) * m_tilewidth + du) <= size[0]);
        ENFORCE((int)((tv + 1) * m_tileheight + dv) <= size[1]);

        return { (int)(tu * m_tilewidth + du), (int)(tv * m_tileheight + dv), (int)m_tilewidth, (int)m_tileheight };
    }
    // -----------------------------------------------------------------------------------------------------
    tile_set tile_set::parse_inner(unsigned first_gid, const reader& elt)
    {
        auto name = elt.get_string_attribute("name", "");
        try {
            auto tilewidth = elt.get_uint_attribute("tilewidth", 0);
            auto tileheight = elt.get_uint_attribute("tileheight", 0);
            auto spacing = elt.get_uint_attribute("spacing", 0);
            auto margin = elt.get_uint_attribute("margin", 0);
            auto tilecount = elt.get_uint_attribute("tilecount", 0);
            auto columns = elt.get_uint_attribute("columns", 0);

            tile_set result(first_gid, name, tilewidth, tileheight, spacing, margin, tilecount, columns);
            component::parse(result, elt);
            elt.parse_one_element("tileoffset", [&result](const reader& elt) {
                int x = elt.get_int_attribute("x");
                int y = elt.get_int_attribute("y");
                result.offset(x, y);
            });
            elt.parse_one_element("grid", [&result] (const reader& elt) {
                 result.add_grid(grid::parse(elt));
            });
            if (const json_reader* jr = dynamic_cast<const json_reader*>(&elt); jr) {
                if (jr->has_element("image")) {
                    result.set_image(image::parse(*jr));
                }
            } else {
                elt.parse_one_element("image", [&result](const reader& e) {
                    result.set_image(image::parse(e));
                });
            }

            elt.parse_one_element("terraintypes", [&result](const reader& e){
                e.parse_many_elements("terrain", [&result](const reader& elt){
                    result.add_terrain(terrain::parse(elt));
                });
            });



            if (dynamic_cast<const xml_reader*>(&elt)) {
                elt.parse_many_elements("tile", [&result](const reader& e){
                    result.add_tile(tile::parse(e));
                });

                elt.parse_one_element("wangsets", [&result](const reader& e){
                    e.parse_many_elements("wangset", [&result](const reader& elt){
                        result.add_wang_set(wang_set::parse(elt));
                    });
                });
            } else {
                elt.parse_one_element("wangsets", [&result](const reader& e){
                    result.add_wang_set(wang_set::parse(e));
                });

                elt.parse_many_elements("tiles", [&result](const reader& e){
                    result.add_tile(tile::parse(e));
                });
            }

            return result;
        } catch (exception& e) {
            RAISE_EX_WITH_CAUSE(std::move(e), "Failed to parse tileset name [", name, "], first_gid [", first_gid, "]");
        }
    }
    // -----------------------------------------------------------------------------------------------------
    tile_set tile_set::parse_from_file(unsigned first_gid, const std::string& source, const path_resolver_t& resolver)
    {
        auto content = resolver(source);
        if (content.empty()) {
            RAISE_EX("Failed to load file  :", source);
        }
        if (content[0] == '<') {
            try {
                return parse_inner(first_gid, xml_reader::load(content.c_str(), content.size(), "tileset"));
            } catch (exception& e) {
                RAISE_EX_WITH_CAUSE(std::move(e), "Failed to parse external tileset [", source, "]");
            }
        } else {
            RAISE_EX("Not Implemented");
        }
    }
    // -----------------------------------------------------------------------------------------------------
    tile_set tile_set::parse(const reader& elt, const path_resolver_t& resolver) {
        auto firstgid = elt.get_uint_attribute("firstgid");
        auto source = elt.get_string_attribute("source", "");
        if (!source.empty()) {
            return parse_from_file(firstgid, source, resolver);
        }
        return parse_inner(firstgid, elt);
    }
}