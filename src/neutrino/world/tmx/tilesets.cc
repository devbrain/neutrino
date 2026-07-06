//
// Created by igor on 05/07/2026.
//

#include "world/tmx/tilesets.hh"

#include "world/tmx/component.hh"
#include "world/tmx/encoding.hh"
#include "world/tmx/image.hh"
#include "world/tmx/objects.hh"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace neutrino::world_tmx {
    namespace {
        [[nodiscard]] world_terrain parse_terrain(const node_view& node) {
            world_terrain result;
            result.name = require_string(node, "name");
            result.tile = get_int(node, "tile", -1);
            parse_local_properties(result, node);
            return result;
        }

        [[nodiscard]] std::array <unsigned, 4> parse_terrain_corners(const node_view& node) {
            std::array <unsigned, 4> terrain{
                world_tile::invalid_terrain,
                world_tile::invalid_terrain,
                world_tile::invalid_terrain,
                world_tile::invalid_terrain
            };

            if (node.is_json()) {
                if (!node.js || !node.js->is_object()) {
                    return terrain;
                }
                const auto it = node.js->find("terrain");
                if (it == node.js->end()) {
                    return terrain;
                }
                if (!it->is_array()) {
                    fail("tile terrain field must be an array");
                }
                std::size_t index = 0;
                for (const auto& item : *it) {
                    if (index >= terrain.size()) {
                        fail("tile terrain array has too many entries");
                    }
                    terrain[index++] = item.get <unsigned>();
                }
                return terrain;
            }

            const auto raw = get_string(node, "terrain", "");
            if (raw.empty()) {
                return terrain;
            }
            std::size_t index = 0;
            std::string token;
            auto flush = [&]() {
                const auto clean = trim(token);
                token.clear();
                if (index >= terrain.size()) {
                    fail("tile terrain attribute has too many entries");
                }
                terrain[index++] = clean.empty()
                    ? world_tile::invalid_terrain
                    : parse_integral <unsigned>(clean, "terrain");
            };
            for (const auto ch : raw) {
                if (ch == ',') {
                    flush();
                } else {
                    token.push_back(ch);
                }
            }
            flush();
            return terrain;
        }

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
            result.terrain = parse_terrain_corners(node);
            result.probability = get_double(node, "probability", 1.0);
            parse_local_properties(result, node);

            if (node.is_json()) {
                if (has_element(node, "image")) {
                    auto image = parse_image(node);
                    if (!image.source.empty()) {
                        result.image = std::move(image);
                    }
                }
            } else {
                for_one_element(node, "image", [&](const node_view& image_node) {
                    result.image = parse_image(image_node);
                });
            }

            result.animation = parse_animation_frames(node);
            for_one_element(node, "objectgroup", [&](const node_view& object_group) {
                result.objects = parse_object_layer(object_group, nullptr);
            });
            return result;
        }

        [[nodiscard]] world_wang_color parse_wang_color(const node_view& node) {
            world_wang_color result;
            result.color = parse_color(require_string(node, "color"));
            result.name = require_string(node, "name");
            result.tile = get_int(node, "tile", -1);
            result.probability = get_double(node, "probability", 0.0);
            parse_local_properties(result, node);
            return result;
        }

        [[nodiscard]] std::array <unsigned, 8> parse_wang_id(const node_view& node) {
            std::array <unsigned, 8> result{};
            if (node.is_json()) {
                if (!node.js || !node.js->is_object()) {
                    fail("wang tile must be a json object");
                }
                const auto it = node.js->find("wangid");
                if (it == node.js->end() || !it->is_array()) {
                    fail("wang tile must contain wangid array");
                }
                if (it->size() != result.size()) {
                    fail("wangid array must have 8 entries");
                }
                for (std::size_t i = 0; i < result.size(); ++i) {
                    result[i] = (*it)[i].get <unsigned>();
                }
                return result;
            }

            const auto values = parse_csv_values(require_string(node, "wangid"));
            if (values.size() != result.size()) {
                fail("wangid attribute must have 8 entries");
            }
            std::copy(values.begin(), values.end(), result.begin());
            return result;
        }

        [[nodiscard]] world_wang_tile parse_wang_tile(const node_view& node) {
            auto result = world_wang_tile{
                parse_wang_id(node),
                require_uint(node, "tileid"),
                sprite_flip::none
            };
            if (get_bool(node, "hflip", false)) {
                result.flip |= sprite_flip::horizontal;
            }
            if (get_bool(node, "vflip", false)) {
                result.flip |= sprite_flip::vertical;
            }
            if (get_bool(node, "dflip", false)) {
                result.flip |= sprite_flip::diagonal;
            }
            return result;
        }

        [[nodiscard]] world_wang_set parse_wang_set(const node_view& node) {
            world_wang_set result;
            result.name = require_string(node, "name");
            result.tile = get_int(node, "tile", -1);
            parse_local_properties(result, node);

            const auto colors_name = node.is_json() ? "colors" : "wangcolor";
            const auto tiles_name = node.is_json() ? "wangtiles" : "wangtile";
            for_each_element(node, colors_name, [&](const node_view& color) {
                result.colors.push_back(parse_wang_color(color));
            });
            for_each_element(node, tiles_name, [&](const node_view& tile) {
                result.tiles.push_back(parse_wang_tile(tile));
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
                    if (!image.source.empty()) {
                        result.image = std::move(image);
                    }
                }
                for_each_element(node, "terrains", [&](const node_view& terrain) {
                    result.terrains.push_back(parse_terrain(terrain));
                });
            } else {
                for_one_element(node, "image", [&](const node_view& image) {
                    result.image = parse_image(image);
                });
                for_one_element(node, "terraintypes", [&](const node_view& terrains) {
                    for_each_element(terrains, "terrain", [&](const node_view& terrain) {
                        result.terrains.push_back(parse_terrain(terrain));
                    });
                });
            }

            const auto tiles_name = node.is_json() ? "tiles" : "tile";
            for_each_element(node, tiles_name, [&](const node_view& tile_node) {
                result.tiles.push_back(parse_tile(tile_node));
            });

            if (node.is_json()) {
                for_each_element(node, "wangsets", [&](const node_view& wang) {
                    result.wang_sets.push_back(parse_wang_set(wang));
                });
            } else {
                for_one_element(node, "wangsets", [&](const node_view& wangsets) {
                    for_each_element(wangsets, "wangset", [&](const node_view& wang) {
                        result.wang_sets.push_back(parse_wang_set(wang));
                    });
                });
            }
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
