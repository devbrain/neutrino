//
// Created by igor on 05/07/2026.
//

#include "world/tmx/component.hh"

#include <filesystem>
#include <string>
#include <utility>

namespace neutrino::world_tmx {
    void parse_properties(
        world_component& out,
        const node_view& node,
        const world_component* inherited) {
        if (inherited != nullptr) {
            for (const auto& [name, value] : inherited->properties()) {
                out.set_property(name, value);
            }
        }

        auto parse_one_property = [&out](const node_view& prop_node) {
            const auto name = require_string(prop_node, "name");
            const auto type = get_string(prop_node, "type", "string");
            auto value = get_string(prop_node, "value", "");
            if (!prop_node.is_json() && value.empty()) {
                value = text(prop_node);
            }

            if (type == "bool") {
                out.set_property(name, parse_bool(value, name));
            } else if (type == "int") {
                out.set_property(name, parse_integral <std::int64_t>(value, name));
            } else if (type == "float") {
                out.set_property(name, parse_floating <float>(value, name));
            } else if (type == "color") {
                out.set_property(name, parse_color(value));
            } else if (type == "file") {
                out.set_property(name, std::filesystem::path(value.empty() ? "." : value));
            } else if (type == "object") {
                out.set_property(name, world_object_reference{parse_integral <world_object_id>(value, name)});
            } else if (type == "string") {
                out.set_property(name, value);
            } else {
                out.set_property(name, world_typed_string{type, value});
            }
        };

        if (node.is_json()) {
            for_each_element(node, "properties", parse_one_property);
        } else {
            for_one_element(node, "properties", [&](const node_view& props) {
                for_each_element(props, "property", parse_one_property);
            });
        }
    }

    void parse_local_properties(world_component& out, const node_view& node) {
        parse_properties(out, node, nullptr);
    }

    group_context parse_group_context(const node_view& node, const group_context* parent) {
        group_context result;
        if (parent != nullptr) {
            result.tint = parent->tint;
        }

        if (parent || has_attribute(node, "offsetx")) {
            result.offset_x = (parent && parent->offset_x ? *parent->offset_x : 0.0) +
                get_double(node, "offsetx", 0.0);
        }
        if (parent || has_attribute(node, "offsety")) {
            result.offset_y = (parent && parent->offset_y ? *parent->offset_y : 0.0) +
                get_double(node, "offsety", 0.0);
        }
        if (parent || has_attribute(node, "opacity")) {
            result.opacity = (parent && parent->opacity ? *parent->opacity : 1.0f) *
                static_cast <float>(get_double(node, "opacity", 1.0));
        }
        if (parent || has_attribute(node, "visible")) {
            result.visible = (parent && parent->visible ? *parent->visible : true) &&
                get_bool(node, "visible", true);
        }
        if (parent || has_attribute(node, "parallaxx")) {
            result.parallax_x = (parent && parent->parallax_x ? *parent->parallax_x : 1.0f) *
                static_cast <float>(get_double(node, "parallaxx", 1.0));
        }
        if (parent || has_attribute(node, "parallaxy")) {
            result.parallax_y = (parent && parent->parallax_y ? *parent->parallax_y : 1.0f) *
                static_cast <float>(get_double(node, "parallaxy", 1.0));
        }
        if (auto tint = parse_optional_color(node, "tintcolor")) {
            result.tint = tint;
        }

        world_component with_parent;
        parse_properties(with_parent, node, parent ? &parent->properties : nullptr);
        result.properties = std::move(with_parent);
        return result;
    }

    world_layer_header parse_layer_header(const node_view& node, const group_context* parent) {
        auto combined = parse_group_context(node, parent);

        world_layer_header result;
        result.id = static_cast <world_layer_id>(get_uint(node, "id", 0));
        result.name = get_string(node, "name", "");
        result.opacity = combined.opacity.value_or(1.0f);
        result.visible = combined.visible.value_or(true);
        result.offset = world_point{
            static_cast <float>(combined.offset_x.value_or(0.0)),
            static_cast <float>(combined.offset_y.value_or(0.0))};
        result.parallax_x = combined.parallax_x.value_or(1.0f);
        result.parallax_y = combined.parallax_y.value_or(1.0f);
        result.tint = combined.tint;
        static_cast <world_component&>(result) = std::move(combined.properties);
        return result;
    }
}
