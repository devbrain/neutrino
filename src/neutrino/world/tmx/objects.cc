//
// Created by igor on 05/07/2026.
//

#include "world/tmx/objects.hh"

#include "world/tmx/encoding.hh"

#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace neutrino::world_tmx {
    namespace {
        [[nodiscard]] std::vector <world_point> parse_xml_points(std::string_view source) {
            std::vector <world_point> result;
            std::string token;
            auto flush = [&]() {
                const auto clean = trim(token);
                token.clear();
                if (clean.empty()) {
                    return;
                }
                const auto comma = clean.find(',');
                if (comma == std::string::npos) {
                    fail("invalid point token <" + clean + ">");
                }
                const auto x = parse_floating <float>(clean.substr(0, comma), "point.x");
                const auto y = parse_floating <float>(clean.substr(comma + 1), "point.y");
                result.emplace_back(x, y);
            };
            for (const auto ch : source) {
                if (std::isspace(static_cast <unsigned char>(ch)) != 0) {
                    flush();
                } else {
                    token.push_back(ch);
                }
            }
            flush();
            return result;
        }

        [[nodiscard]] std::vector <world_point> parse_json_points(
            const node_view& object,
            std::string_view name) {
            std::vector <world_point> result;
            if (!object.is_json() || !object.js || !object.js->is_object()) {
                return result;
            }
            const auto it = object.js->find(make_string(name));
            if (it == object.js->end() || !it->is_array()) {
                fail("missing point array <" + make_string(name) + ">");
            }
            for (const auto& point : *it) {
                if (!point.is_object()) {
                    fail("point array entries must be objects");
                }
                result.emplace_back(
                    point.at("x").get <float>(),
                    point.at("y").get <float>());
            }
            return result;
        }

        [[nodiscard]] std::vector <world_point> parse_points(
            const node_view& object,
            std::string_view child_name) {
            if (object.is_json()) {
                return parse_json_points(object, child_name);
            }

            std::vector <world_point> result;
            for_one_element(object, child_name, [&](const node_view& child) {
                result = parse_xml_points(require_string(child, "points"));
            });
            return result;
        }

        [[nodiscard]] world_object_base parse_object_base(const node_view& node) {
            world_object_base result;
            result.id = get_int(node, "id", -1);
            result.name = get_string(node, "name", "");
            result.type = get_string(node, "type", "");
            result.origin = {
                static_cast <float>(get_double(node, "x", 0.0)),
                static_cast <float>(get_double(node, "y", 0.0))
            };
            result.width = get_double(node, "width", 0.0);
            result.height = get_double(node, "height", 0.0);
            result.rotation = get_double(node, "rotation", 0.0);
            result.visible = get_bool(node, "visible", true);
            const auto decoded = decode_gid(get_uint(node, "gid", 0));
            result.gid = decoded.gid;
            result.flip = decoded.flip;
            result.rotation += decoded.rotation_degrees; // fold TMX hex-120 into object rotation
            parse_local_properties(result, node);
            return result;
        }

        template <typename T>
        [[nodiscard]] T with_base(world_object_base base) {
            T object;
            static_cast <world_object_base&>(object) = std::move(base);
            return object;
        }

        [[nodiscard]] bool object_discriminator(const node_view& node, std::string_view name) {
            if (node.is_json()) {
                return get_bool(node, name, false);
            }
            return has_element(node, name);
        }
    }

    world_object parse_object(const node_view& node) {
        auto base = parse_object_base(node);

        if (has_element(node, "polygon")) {
            auto object = with_base <world_polygon_object>(std::move(base));
            object.points = parse_points(node, "polygon");
            return object;
        }
        if (has_element(node, "polyline")) {
            auto object = with_base <world_polyline_object>(std::move(base));
            object.points = parse_points(node, "polyline");
            return object;
        }
        if (object_discriminator(node, "ellipse")) {
            return with_base <world_ellipse_object>(std::move(base));
        }
        if (object_discriminator(node, "point")) {
            return with_base <world_point_object>(std::move(base));
        }
        if (has_element(node, "text")) {
            auto object = with_base <world_text_object>(std::move(base));
            for_one_element(node, "text", [&](const node_view& text_node) {
                object.font_family = get_string(text_node, "fontfamily", "sans-serif");
                object.pixel_size = get_int(text_node, "pixelsize", 16);
                object.wrap = get_bool(text_node, "wrap", false);
                object.color = parse_color(get_string(text_node, "color", "#000000"));
                object.bold = get_bool(text_node, "bold", false);
                object.italic = get_bool(text_node, "italic", false);
                object.underline = get_bool(text_node, "underline", false);
                object.strike = get_bool(text_node, "strikeout", false);
                object.kerning = get_bool(text_node, "kerning", true);
                object.halign = parse_enum <world_text_halign>(
                    text_node,
                    "halign",
                    world_text_halign::left,
                    {
                        {"left", world_text_halign::left},
                        {"center", world_text_halign::center},
                        {"right", world_text_halign::right},
                        {"justify", world_text_halign::justify}
                    });
                object.valign = parse_enum <world_text_valign>(
                    text_node,
                    "valign",
                    world_text_valign::top,
                    {
                        {"top", world_text_valign::top},
                        {"center", world_text_valign::center},
                        {"bottom", world_text_valign::bottom}
                    });
                object.text = text_node.is_json()
                    ? get_string(text_node, "text", "")
                    : text(text_node);
            });
            return object;
        }
        return with_base <world_rectangle_object>(std::move(base));
    }

    world_object_layer parse_object_layer(const node_view& node, const group_context* parent) {
        world_object_layer result;
        static_cast <world_layer_header&>(result) = parse_layer_header(node, parent);
        result.color = parse_color(get_string(node, "color", "#a0a0a4"));
        result.draw_order = parse_enum <world_object_draw_order>(
            node,
            "draworder",
            world_object_draw_order::top_down,
            {
                {"topdown", world_object_draw_order::top_down},
                {"index", world_object_draw_order::index}
            });

        const auto objects_name = node.is_json() ? "objects" : "object";
        for_each_element(node, objects_name, [&](const node_view& object_node) {
            result.objects.push_back(parse_object(object_node));
        });
        return result;
    }
}
