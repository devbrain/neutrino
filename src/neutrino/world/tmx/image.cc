//
// Created by igor on 05/07/2026.
//

#include "world/tmx/image.hh"

#include "world/tmx/encoding.hh"

namespace neutrino::world_tmx {
    world_image parse_image(const node_view& node) {
        world_image result;
        if (node.is_json()) {
            result.source = image_from_disk{get_string(node, "image", "")};
            result.transparent_color = parse_optional_color(node, "transparentcolor");
            result.width = get_uint(node, "imagewidth", 0);
            result.height = get_uint(node, "imageheight", 0);
            return result;
        }

        result.transparent_color = parse_optional_color(node, "trans");
        result.width = get_uint(node, "width", 0);
        result.height = get_uint(node, "height", 0);

        // An embedded <data> image carries its (encoded) bytes in memory; otherwise the
        // image is an external file referenced by `source`. The Tiled `format` attribute
        // is not read at runtime, so it is intentionally dropped.
        std::optional <std::vector <std::uint8_t>> embedded;
        for_one_element(node, "data", [&](const node_view& data_node) {
            const auto encoding = get_string(data_node, "encoding", "");
            const auto compression = get_string(data_node, "compression", "");
            embedded = parse_encoded_bytes(encoding, compression, text(data_node));
        });
        if (embedded) {
            result.source = image_from_memory{std::move(*embedded)};
        } else {
            result.source = image_from_disk{get_string(node, "source", "")};
        }
        return result;
    }
}
