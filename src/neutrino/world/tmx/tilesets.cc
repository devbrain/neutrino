//
// Created by igor on 05/07/2026.
//

#include "world/tmx/tilesets.hh"

#include "world/tmx/component.hh"
#include "world/tmx/encoding.hh"
#include "world/tmx/image.hh"
#include "world/tmx/objects.hh"

#include <chrono>
#include <string>
#include <vector>

namespace neutrino::world_tmx {
    namespace {
        [[nodiscard]] std::vector <world_tile_animation_frame> parse_animation_frames(
            const node_view& node) {
            std::vector <world_tile_animation_frame> frames;
            auto parse_frame = [&frames](const node_view& frame) {
                frames.push_back({
                    static_cast <world_local_tile_id>(require_uint(frame, "tileid")),
                    std::chrono::milliseconds{require_uint(frame, "duration")}
                });
            };

            if (node.is_json()) {
                for_each_element(node, "animation", parse_frame);
            } else {
                for_one_element(node, "animation", [&](const node_view& animation) {
                    for_each_element(animation, "frame", parse_frame);
                });
            }
            return frames;
        }

        [[nodiscard]] world_tile parse_tile(const node_view& node) {
            world_tile result;
            result.id = static_cast <world_local_tile_id>(require_uint(node, "id"));
            parse_local_properties(result, node);

            if (node.is_json()) {
                if (has_element(node, "image")) {
                    auto image = parse_image(node);
                    if (!image.empty()) {
                        result.image = std::move(image);
                    }
                }
            } else {
                for_one_element(node, "image", [&](const node_view& image_node) {
                    result.image = parse_image(image_node);
                });
            }

            // Image-collection atlas: an x/y/width/height on the tile is the tile's
            // sub-rectangle within its (shared) image.
            if (result.image) {
                const auto rect_w = get_uint(node, "width", 0);
                const auto rect_h = get_uint(node, "height", 0);
                if (rect_w > 0 && rect_h > 0) {
                    result.source_rect = rect{
                        static_cast <int>(get_uint(node, "x", 0)),
                        static_cast <int>(get_uint(node, "y", 0)),
                        static_cast <int>(rect_w),
                        static_cast <int>(rect_h)
                    };
                }
            }

            result.animation = parse_animation_frames(node);
            for_one_element(node, "objectgroup", [&](const node_view& object_group) {
                result.objects = parse_object_layer(object_group, nullptr);
            });
            return result;
        }

        [[nodiscard]] std::string resolve_required(
            const tmx_path_resolver& resolver,
            const std::string& source) {
            if (!resolver) {
                fail("external tileset <" + source + "> requires a path resolver");
            }
            auto content = resolver(source);
            if (content.empty()) {
                fail("external tileset resolver returned no data for <" + source + ">");
            }
            return content;
        }

        [[nodiscard]] world_tileset parse_tileset_inner(
            unsigned first_gid,
            const node_view& node,
            const tmx_path_resolver& resolver);

        [[nodiscard]] world_tileset parse_external_tileset(
            unsigned first_gid,
            const std::string& source,
            const tmx_path_resolver& resolver) {
            const auto content = resolve_required(resolver, source);
            const auto clean = utils::trim_document(content);
            if (clean.empty()) {
                fail("external tileset <" + source + "> is empty");
            }

            if (clean.front() == '<') {
                pugi::xml_document doc;
                const auto status = doc.load_buffer(clean.data(), clean.size());
                if (!status) {
                    fail("failed to parse external TSX <" + source + ">: " + status.description());
                }
                const auto root = doc.child("tileset");
                if (!root) {
                    fail("external TSX <" + source + "> has no <tileset> root");
                }
                return parse_tileset_inner(first_gid, node_view{node_kind::xml, root, nullptr}, resolver);
            }

            try {
                json doc = json::parse(clean);
                const json* root = &doc;
                if (doc.is_object()) {
                    if (const auto it = doc.find("tileset"); it != doc.end()) {
                        root = &*it;
                    }
                }
                return parse_tileset_inner(first_gid, node_view{node_kind::json, {}, root}, resolver);
            } catch (const json::exception& e) {
                fail("failed to parse external JSON tileset <" + source + ">: " + e.what());
            }
        }

        [[nodiscard]] world_tileset parse_tileset_inner(
            unsigned first_gid,
            const node_view& node,
            const tmx_path_resolver& resolver) {
            static_cast <void>(resolver);
            world_tileset result;
            result.first_gid = first_gid;
            result.name = get_string(node, "name", "");
            result.tile_width = get_uint(node, "tilewidth", 0);
            result.tile_height = get_uint(node, "tileheight", 0);
            result.spacing = get_uint(node, "spacing", 0);
            result.margin = get_uint(node, "margin", 0);
            result.tile_count = get_uint(node, "tilecount", 0);
            result.columns = get_uint(node, "columns", 0);
            parse_local_properties(result, node);

            for_one_element(node, "tileoffset", [&](const node_view& offset) {
                result.offset_x = get_int(offset, "x", 0);
                result.offset_y = get_int(offset, "y", 0);
            });

            for_one_element(node, "grid", [&](const node_view& grid) {
                result.grid = world_tileset_grid{
                    get_string(grid, "orientation", "orthogonal") == "orthogonal",
                    require_uint(grid, "width"),
                    require_uint(grid, "height")
                };
            });

            if (node.is_json()) {
                if (has_attribute(node, "image")) {
                    auto image = parse_image(node);
                    if (!image.empty()) {
                        result.image = std::move(image);
                    }
                }
            } else {
                for_one_element(node, "image", [&](const node_view& image) {
                    result.image = parse_image(image);
                });
            }

            // Older/hand-authored TMX may omit columns/tilecount for a uniform (shared
            // image) tileset; Tiled derives them from the image. Do the same, or the
            // tileset builds zero visuals and every cell referencing it is skipped.
            if (result.image && result.tile_width > 0 && result.tile_height > 0) {
                const unsigned iw = result.image->width;
                const unsigned ih = result.image->height;
                const unsigned two_margin = result.margin * 2;
                if (result.columns == 0 && iw > two_margin) {
                    result.columns = (iw - two_margin + result.spacing) / (result.tile_width + result.spacing);
                }
                if (result.tile_count == 0 && result.columns > 0 && ih > two_margin) {
                    const unsigned rows = (ih - two_margin + result.spacing) / (result.tile_height + result.spacing);
                    result.tile_count = result.columns * rows;
                }
            }

            // Tiled terrain/wang authoring metadata is intentionally not parsed:
            // it drives the editor's autotiling, is never read at runtime, and has
            // no analog in other loaders (e.g. LDtk). The world model stays neutral.
            const auto tiles_name = node.is_json() ? "tiles" : "tile";
            for_each_element(node, tiles_name, [&](const node_view& tile_node) {
                result.tiles.push_back(parse_tile(tile_node));
            });
            return result;
        }
    }

    world_tileset parse_tileset(const node_view& node, const tmx_path_resolver& resolver) {
        const auto first_gid = require_uint(node, "firstgid");
        const auto source = get_string(node, "source", "");
        if (!source.empty()) {
            return parse_external_tileset(first_gid, source, resolver);
        }
        return parse_tileset_inner(first_gid, node, resolver);
    }
}
