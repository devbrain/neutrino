//
// Created by igor on 05/07/2026.
//

#include "world/tmx/layers.hh"

#include "world/tmx/encoding.hh"
#include "world/tmx/image.hh"
#include "world/tmx/objects.hh"

#include <limits>
#include <string>
#include <utility>

namespace neutrino::world_tmx {
    namespace {
        [[nodiscard]] std::size_t checked_product(
            std::size_t width,
            std::size_t height,
            std::string_view what) {
            if (height != 0 && width > std::numeric_limits <std::size_t>::max() / height) {
                fail(make_string(what) + " cell count is too large");
            }
            return width * height;
        }

        [[nodiscard]] std::size_t expected_cell_count(
            unsigned width,
            unsigned height,
            std::string_view what) {
            return checked_product(width, height, what);
        }

        [[nodiscard]] std::size_t expected_cell_count(
            int width,
            int height,
            std::string_view what) {
            if (width < 0 || height < 0) {
                fail(make_string(what) + " dimensions must not be negative");
            }
            return checked_product(
                static_cast <std::size_t>(width),
                static_cast <std::size_t>(height),
                what);
        }

        void validate_cell_count(
            std::size_t actual,
            std::size_t expected,
            std::string_view what) {
            if (actual != expected) {
                fail(
                    make_string(what) + " contains " + std::to_string(actual) +
                    " cells, expected " + std::to_string(expected));
            }
        }

        [[nodiscard]] std::vector <world_tile_cell> parse_json_gid_array(
            const json& data,
            std::size_t expected_cells,
            std::string_view what) {
            if (!data.is_array()) {
                fail(make_string(what) + " data must be an array");
            }
            std::vector <world_tile_cell> result;
            result.reserve(data.size());
            for (const auto& gid : data) {
                result.push_back(decode_gid(gid.get <std::uint32_t>()));
            }
            validate_cell_count(result.size(), expected_cells, what);
            return result;
        }

        [[nodiscard]] std::vector <world_tile_cell> parse_json_tile_data(
            const node_view& node,
            const std::string& encoding,
            const std::string& compression,
            std::size_t expected_cells,
            std::string_view what) {
            if (!node.js || !node.js->is_object()) {
                fail(make_string(what) + " must be a json object");
            }

            const auto data = node.js->find("data");
            if (data == node.js->end()) {
                fail(make_string(what) + " has no data");
            }
            if (data->is_array() && compression.empty() && (encoding.empty() || encoding == "csv")) {
                return parse_json_gid_array(*data, expected_cells, what);
            }
            if (!data->is_string()) {
                fail(make_string(what) + " encoded data must be a string");
            }
            return parse_encoded_cells(encoding, compression, data->get <std::string>(), expected_cells);
        }

        [[nodiscard]] world_tile_chunk parse_chunk(
            const node_view& node,
            const std::string& encoding,
            const std::string& compression) {
            world_tile_chunk result;
            result.x = require_int(node, "x");
            result.y = require_int(node, "y");
            result.width = require_int(node, "width");
            result.height = require_int(node, "height");
            const auto expected_cells = expected_cell_count(result.width, result.height, "chunk");

            if (node.is_json()) {
                result.cells = parse_json_tile_data(node, encoding, compression, expected_cells, "chunk");
            } else {
                if (encoding.empty() && compression.empty()) {
                    for_each_element(node, "tile", [&](const node_view& tile) {
                        result.cells.push_back(decode_gid(get_uint(tile, "gid", 0)));
                    });
                    validate_cell_count(result.cells.size(), expected_cells, "chunk");
                } else {
                    result.cells = parse_encoded_cells(encoding, compression, text(node), expected_cells);
                }
            }
            return result;
        }

        [[nodiscard]] world_tile_layer parse_tile_layer(const node_view& node, const group_context* parent) {
            world_tile_layer result;
            static_cast <world_layer_header&>(result) = parse_layer_header(node, parent);
            result.width = get_uint(node, "width", 0);
            result.height = get_uint(node, "height", 0);
            const auto expected_cells = expected_cell_count(result.width, result.height, "tile layer");

            if (node.is_json()) {
                const auto encoding = get_string(node, "encoding", "");
                const auto compression = get_string(node, "compression", "");
                if (has_element(node, "chunks")) {
                    for_each_element(node, "chunks", [&](const node_view& chunk) {
                        result.chunks.push_back(parse_chunk(chunk, encoding, compression));
                    });
                } else {
                    result.cells = parse_json_tile_data(node, encoding, compression, expected_cells, "tile layer");
                }
                return result;
            }

            for_one_element(node, "data", [&](const node_view& data_node) {
                const auto encoding = get_string(data_node, "encoding", "");
                const auto compression = get_string(data_node, "compression", "");
                if (has_element(data_node, "chunk")) {
                    for_each_element(data_node, "chunk", [&](const node_view& chunk) {
                        result.chunks.push_back(parse_chunk(chunk, encoding, compression));
                    });
                } else if (encoding.empty() && compression.empty()) {
                    for_each_element(data_node, "tile", [&](const node_view& tile) {
                        result.cells.push_back(decode_gid(get_uint(tile, "gid", 0)));
                    });
                    validate_cell_count(result.cells.size(), expected_cells, "tile layer");
                } else {
                    result.cells = parse_encoded_cells(encoding, compression, text(data_node), expected_cells);
                }
            });
            return result;
        }

        [[nodiscard]] world_image_layer parse_image_layer(const node_view& node, const group_context* parent) {
            world_image_layer result;
            static_cast <world_layer_header&>(result) = parse_layer_header(node, parent);

            if (node.is_json()) {
                if (has_attribute(node, "image")) {
                    auto image = parse_image(node);
                    if (!image.source.empty()) {
                        result.image = std::move(image);
                    }
                }
            } else {
                for_one_element(node, "image", [&](const node_view& image) {
                    result.image = parse_image(image);
                });
            }
            return result;
        }

        void parse_layer_node(const node_view& node, world& out, const group_context* parent) {
            if (node.is_json()) {
                const auto type = require_string(node, "type");
                if (type == "tilelayer") {
                    out.add_layer(parse_tile_layer(node, parent));
                } else if (type == "objectgroup") {
                    out.add_layer(parse_object_layer(node, parent));
                } else if (type == "imagelayer") {
                    out.add_layer(parse_image_layer(node, parent));
                } else if (type == "group") {
                    const auto current = parse_group_context(node, parent);
                    parse_layers(node, out, &current);
                } else {
                    fail("unknown layer type <" + type + ">");
                }
                return;
            }

            const auto name = std::string(node.xml.name());
            if (name == "layer") {
                out.add_layer(parse_tile_layer(node, parent));
            } else if (name == "objectgroup") {
                out.add_layer(parse_object_layer(node, parent));
            } else if (name == "imagelayer") {
                out.add_layer(parse_image_layer(node, parent));
            } else if (name == "group") {
                const auto current = parse_group_context(node, parent);
                parse_layers(node, out, &current);
            }
        }
    }

    void parse_layers(const node_view& node, world& out, const group_context* parent) {
        if (node.is_json()) {
            for_each_element(node, "layers", [&](const node_view& layer) {
                parse_layer_node(layer, out, parent);
            });
            return;
        }

        for (auto child = node.xml.first_child(); child; child = child.next_sibling()) {
            parse_layer_node(node_view{node_kind::xml, child, nullptr}, out, parent);
        }
    }
}
