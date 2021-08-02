//
// Created by igor on 21/07/2021.
//

#include "map.hh"
#include "tile_layer.hh"
#include <neutrino/utils/exception.hh>

namespace neutrino::tiled::tmx
{
    map map::parse(const xml_node& elt, path_resolver_t resolver) {
        std::string version =  elt.get_string_attribute("version", Requirement::OPTIONAL, "1.0");

        static const std::map<std::string, orientation_t> orientation_mp = {
                {"orthogonal", orientation_t::ORTHOGONAL},
                {"isometric", orientation_t::ISOMETRIC},
                {"staggered", orientation_t::STAGGERED},
                {"hexagonal", orientation_t::HEXAGONAL},
        };

        auto orientation = elt.parse_enum("orientation", orientation_t::UNKNOWN, orientation_mp);
        auto width = elt.get_attribute<unsigned>("width");
        auto height = elt.get_attribute<unsigned>("height");
        auto tilewidth = elt.get_attribute<unsigned>("tilewidth");
        auto tileheight = elt.get_attribute<unsigned>("tileheight");
        auto bgcolor = elt.get_attribute("backgroundcolor", Requirement::OPTIONAL, "#000000");

        static const std::map<std::string, render_order_t> render_order_mp = {
                {"right-down", render_order_t::RIGHT_DOWN},
                {"right-up",   render_order_t::RIGHT_UP},
                {"left-down",  render_order_t::LEFT_DOWN},
                {"left-up",    render_order_t::LEFT_UP},
        };
        auto render_order = elt.parse_enum("renderorder", render_order_t::RIGHT_DOWN, render_order_mp);

        auto side_length = elt.get_attribute<unsigned>("hexsidelength", Requirement::OPTIONAL);

        static const std::map<std::string, stagger_axis_t> stagger_axis_mp = {
                {"x", stagger_axis_t::X},
                {"y", stagger_axis_t::Y}
        };
        auto axis = elt.parse_enum("staggeraxis", stagger_axis_t::Y, stagger_axis_mp);
        static const std::map<std::string, stagger_index_t> stagger_index_mp = {
                {"odd", stagger_index_t::ODD},
                {"even", stagger_index_t::EVEN}
        };

        auto index = elt.parse_enum("staggerindex", stagger_index_t::ODD, stagger_index_mp);

        auto infinite = elt.get_attribute<bool>("infinite", Requirement::OPTIONAL, false);

        map result(version, orientation, width, height, tilewidth, tileheight, colori(bgcolor), render_order,
                   side_length, axis, index, infinite);

        elt.parse_many_elements("tileset", [&result, &resolver](const xml_node& e) {
            result.m_tilesets.push_back(tile_set::parse(e, resolver));
        });

        component::parse(result, elt);
        parse_group(elt, result, nullptr, resolver);

        return result;
    }
    // ------------------------------------------------------------------------------------------
    const tile_set* map::tile_set_from_gid(unsigned int gid) const noexcept
    {
        for (auto i = m_tilesets.rbegin(); i != m_tilesets.rend(); i++)
        {
            const auto& tileset = *i;
            if (tileset.first_gid() <= gid)
            {
                return &tileset;
            }
        }
        return nullptr;
    }
    // -------------------------------------------------------------------------------------------
    void map::parse_group(const xml_node& elt, map& result, const group* parent, path_resolver_t resolver) {
        elt.parse_many_elements("layer", [&result, &resolver, parent](const xml_node& e) {
            result.m_layers.emplace_back(tile_layer::parse(e, parent));
        });
        elt.parse_many_elements("objectgroup", [&result, &resolver, parent](const xml_node& e) {
            result.m_object_layers.emplace_back(object_layer::parse(e, parent));
        });
        elt.parse_many_elements("imagelayer", [&result, &resolver, parent](const xml_node& e) {
            result.m_layers.emplace_back(image_layer::parse(e, parent));
        });
        elt.parse_many_elements("group", [&result, &resolver, parent](const xml_node& e) {
                auto current = group::parse(e, parent);
                parse_group(e, result, &current, resolver);
        });
    }
}
