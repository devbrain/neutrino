//
// Created by igor on 05/07/2026.
//

#include "world/tmx/world_parser.hh"

#include "world/tmx/component.hh"
#include "world/tmx/layers.hh"
#include "world/tmx/tilesets.hh"

namespace neutrino::world_tmx {
    world parse_world(const node_view& node, const tmx_path_resolver& resolver) {
        world result;
        result.set_version(get_string(node, "version", "1.0"));
        result.set_orientation(parse_enum <world_orientation>(
            node,
            "orientation",
            world_orientation::unknown,
            {
                {"orthogonal", world_orientation::orthogonal},
                {"isometric", world_orientation::isometric},
                {"staggered", world_orientation::staggered},
                {"hexagonal", world_orientation::hexagonal}
            }));
        result.set_render_order(parse_enum <world_render_order>(
            node,
            "renderorder",
            world_render_order::right_down,
            {
                {"right-down", world_render_order::right_down},
                {"right-up", world_render_order::right_up},
                {"left-down", world_render_order::left_down},
                {"left-up", world_render_order::left_up}
            }));
        result.set_size(require_uint(node, "width"), require_uint(node, "height"));
        result.set_tile_size(require_uint(node, "tilewidth"), require_uint(node, "tileheight"));
        result.set_background_color(parse_color(get_string(node, "backgroundcolor", "#000000")));
        result.set_hex_side_length(get_uint(node, "hexsidelength", 0));
        result.set_stagger_axis(parse_enum <world_stagger_axis>(
            node,
            "staggeraxis",
            world_stagger_axis::y,
            {
                {"x", world_stagger_axis::x},
                {"y", world_stagger_axis::y}
            }));
        result.set_stagger_index(parse_enum <world_stagger_index>(
            node,
            "staggerindex",
            world_stagger_index::odd,
            {
                {"odd", world_stagger_index::odd},
                {"even", world_stagger_index::even}
            }));
        result.set_infinite(get_bool(node, "infinite", false));
        parse_local_properties(result, node);

        const auto tilesets_name = node.is_json() ? "tilesets" : "tileset";
        for_each_element(node, tilesets_name, [&](const node_view& tileset) {
            result.add_tileset(parse_tileset(tileset, resolver));
        });

        parse_layers(node, result, nullptr);
        return result;
    }
}
