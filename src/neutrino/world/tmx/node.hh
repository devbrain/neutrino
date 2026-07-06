//
// Created by igor on 05/07/2026.
//

#pragma once

#include "utils/json.hh"

#include <pugixml.hpp>

#include <charconv>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <neutrino/world/world.hh>

namespace neutrino::world_tmx {
    using json = utils::json;

    enum class node_kind {
        xml,
        json
    };

    struct node_view {
        node_kind kind;
        pugi::xml_node xml;
        const json* js{};
        const std::string* json_key{};

        [[nodiscard]] bool is_json() const noexcept {
            return kind == node_kind::json;
        }
    };

    [[noreturn]] void fail(const std::string& message);

    [[nodiscard]] std::string make_string(std::string_view value);
    [[nodiscard]] std::string trim(std::string_view value);

    [[nodiscard]] std::optional <std::string> attribute(const node_view& node, std::string_view name);
    [[nodiscard]] bool has_attribute(const node_view& node, std::string_view name);
    [[nodiscard]] bool has_element(const node_view& node, std::string_view name);
    [[nodiscard]] std::string text(const node_view& node);

    template <typename Fn>
    void for_each_element(const node_view& node, std::string_view name, Fn&& fn) {
        if (node.is_json()) {
            if (!node.js || !node.js->is_object()) {
                return;
            }
            const auto it = node.js->find(make_string(name));
            if (it == node.js->end()) {
                return;
            }
            if (it->is_array()) {
                for (const auto& item : *it) {
                    fn(node_view{node_kind::json, {}, &item});
                }
                return;
            }
            if (it->is_object()) {
                for (auto item = it->begin(); item != it->end(); ++item) {
                    fn(node_view{node_kind::json, {}, &*item, &item.key()});
                }
                return;
            }
            fail("json field <" + make_string(name) + "> must be an object or array");
        }

        for (auto child = node.xml.child(make_string(name).c_str()); child;
             child = child.next_sibling(make_string(name).c_str())) {
            fn(node_view{node_kind::xml, child, nullptr});
        }
    }

    template <typename Fn>
    void for_one_element(const node_view& node, std::string_view name, Fn&& fn) {
        if (node.is_json()) {
            if (!node.js || !node.js->is_object()) {
                return;
            }
            const auto it = node.js->find(make_string(name));
            if (it == node.js->end()) {
                return;
            }
            if (it->is_object()) {
                fn(node_view{node_kind::json, {}, &*it});
                return;
            }
            if (it->is_array()) {
                for (const auto& item : *it) {
                    fn(node_view{node_kind::json, {}, &item});
                }
                return;
            }
            fail("json field <" + make_string(name) + "> is not iterable");
        }

        const auto child = node.xml.child(make_string(name).c_str());
        if (child) {
            fn(node_view{node_kind::xml, child, nullptr});
        }
    }

    [[nodiscard]] std::string get_string(
        const node_view& node,
        std::string_view name,
        std::string_view default_value);
    [[nodiscard]] std::string require_string(const node_view& node, std::string_view name);

    template <typename T>
    [[nodiscard]] T parse_integral(std::string_view value, std::string_view name) {
        const auto clean = trim(value);
        T result{};
        const auto* first = clean.data();
        const auto* last = clean.data() + clean.size();
        const auto [ptr, ec] = std::from_chars(first, last, result);
        if (ec != std::errc{} || ptr != last) {
            fail("failed to parse integer attribute <" + make_string(name) + "> from <" + clean + ">");
        }
        return result;
    }

    template <typename T>
    [[nodiscard]] T parse_floating(std::string_view value, std::string_view name) {
        const auto clean = trim(value);
        T result{};
        const auto* first = clean.data();
        const auto* last = clean.data() + clean.size();
        const auto [ptr, ec] = std::from_chars(first, last, result);
        if (ec != std::errc{} || ptr != last) {
            fail("failed to parse floating-point attribute <" + make_string(name) + "> from <" + clean + ">");
        }
        return result;
    }

    [[nodiscard]] unsigned get_uint(const node_view& node, std::string_view name, unsigned default_value);
    [[nodiscard]] unsigned require_uint(const node_view& node, std::string_view name);
    [[nodiscard]] int get_int(const node_view& node, std::string_view name, int default_value);
    [[nodiscard]] int require_int(const node_view& node, std::string_view name);
    [[nodiscard]] double get_double(const node_view& node, std::string_view name, double default_value);
    [[nodiscard]] bool parse_bool(std::string_view value, std::string_view name);
    [[nodiscard]] bool get_bool(const node_view& node, std::string_view name, bool default_value);

    template <typename T>
    [[nodiscard]] T parse_enum(
        const node_view& node,
        std::string_view name,
        T default_value,
        std::initializer_list <std::pair <std::string_view, T>> values) {
        const auto raw = attribute(node, name);
        if (!raw) {
            return default_value;
        }
        for (const auto& [key, value] : values) {
            if (*raw == key) {
                return value;
            }
        }
        fail("unknown value <" + *raw + "> for attribute <" + make_string(name) + ">");
    }

    [[nodiscard]] sdlpp::color parse_color(std::string_view raw);
    [[nodiscard]] std::optional <sdlpp::color> parse_optional_color(
        const node_view& node,
        std::string_view name);
}
