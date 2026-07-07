//
// Created by igor on 05/07/2026.
//

#include "world/tmx/image.hh"

#include "world/tmx/encoding.hh"

namespace neutrino::world_tmx {
    world_image parse_image(const node_view& node) {
        world_image result;
        if (node.is_json()) {
            result.source = get_string(node, "image", "");
            result.transparent_color = parse_optional_color(node, "transparentcolor");
            result.width = get_uint(node, "imagewidth", 0);
            result.height = get_uint(node, "imageheight", 0);
            return result;
        }

        result.format = get_string(node, "format", "");
        result.source = get_string(node, "source", "");
        result.transparent_color = parse_optional_color(node, "trans");
        result.width = get_uint(node, "width", 0);
        result.height = get_uint(node, "height", 0);
        for_one_element(node, "data", [&](const node_view& data_node) {
            const auto encoding = get_string(data_node, "encoding", "");
            const auto compression = get_string(data_node, "compression", "");
            result.data = parse_encoded_bytes(encoding, compression, text(data_node));
        });
        return result;
    }
}
