//
// Created by igor on 20/07/2021.
//

#include "tile_set.hh"
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
    tile_set tile_set::parse_inner(unsigned first_gid, const xml_node& elt)
    {
        auto name = elt.get_string_attribute("name", Requirement::OPTIONAL);
        try {
            auto tilewidth = elt.get_uint_attribute("tilewidth", Requirement::OPTIONAL);
            auto tileheight = elt.get_uint_attribute("tileheight", Requirement::OPTIONAL);
            auto spacing = elt.get_uint_attribute("spacing", Requirement::OPTIONAL);
            auto margin = elt.get_uint_attribute("margin", Requirement::OPTIONAL);
            auto tilecount = elt.get_uint_attribute("tilecount", Requirement::OPTIONAL);

            tile_set result(first_gid, name, tilewidth, tileheight, spacing, margin, tilecount);
            component::parse(result, elt);
            elt.parse_one_element("tileoffset", [&result](const xml_node& elt) {
                int x = elt.get_int_attribute("x");
                int y = elt.get_int_attribute("y");
                result.offset(x, y);
            });
            elt.parse_one_element("image", [&result](const xml_node& e) {
                result.set_image(image::parse(e));
            });
            elt.parse_one_element("terraintypes", [&result](const xml_node& e){
                e.parse_many_elements("terrain", [&result](const xml_node& elt){
                    result.add_terrain(terrain::parse(elt));
                });
            });

            elt.parse_many_elements("tile", [&result](const xml_node& e){
                result.add_tile(tile::parse(e));
            });

            return result;
        } catch (exception& e) {
            RAISE_EX_WITH_CAUSE(std::move(e), "Failed to parse tileset name [", name, "], first_gid [", first_gid, "]");
        }
    }
    // -----------------------------------------------------------------------------------------------------
    tile_set tile_set::parse_from_file(unsigned first_gid, const std::string& source, const path_resolver_t& resolver)
    {
        auto content = resolver(source);
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_string(content.c_str());
        if (!result)
        {
            RAISE_EX("Failed to load file  :", source, " : " , result.description());
        }
        auto root = doc.child("tileset");
        if (!root)
        {
            RAISE_EX ("entry node <tileset> is missing");
        }
        try {
            return parse_inner(first_gid, xml_node(root));
        } catch (exception& e) {
            RAISE_EX_WITH_CAUSE(std::move(e), "Failed to parse external tileset [", source, "]");
        }
    }
    // -----------------------------------------------------------------------------------------------------
    tile_set tile_set::parse(const xml_node& elt, const path_resolver_t& resolver) {
        auto firstgid = elt.get_attribute<unsigned>("firstgid");
        auto source = elt.get_string_attribute("source", Requirement::OPTIONAL);
        if (!source.empty()) {
            return parse_from_file(firstgid, source, resolver);
        }
        return parse_inner(firstgid, elt);
    }
}