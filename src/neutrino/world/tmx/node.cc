//
// Created by igor on 05/07/2026.
//

#include "world/tmx/node.hh"

#include <failsafe/exception.hh>

#include <cctype>

namespace neutrino::world_tmx {
    [[noreturn]] void fail(const std::string& message) {
        THROW_RUNTIME("TMX parse error: " + message);
    }

    std::string make_string(std::string_view value) {
        return {value.data(), value.size()};
    }

    std::string trim(std::string_view value) {
        auto first = value.begin();
        auto last = value.end();
        while (first != last && std::isspace(static_cast <unsigned char>(*first))) {
            ++first;
        }
        while (first != last) {
            auto prev = last;
            --prev;
            if (!std::isspace(static_cast <unsigned char>(*prev))) {
                break;
            }
            last = prev;
        }
        return {first, last};
    }

    std::optional <std::string> attribute(const node_view& node, std::string_view name) {
        if (node.is_json()) {
            if (node.json_key != nullptr && (name == "name" || name == "id")) {
                return *node.json_key;
            }
            if (!node.js || !node.js->is_object()) {
                if (node.js && name == "value" && !node.js->is_null() && !node.js->is_object() && !node.js->is_array()) {
                    if (node.js->is_string()) {
                        return node.js->get <std::string>();
                    }
                    return node.js->dump();
                }
                return std::nullopt;
            }
            const auto it = node.js->find(make_string(name));
            if (it == node.js->end() || it->is_null() || it->is_object() || it->is_array()) {
                return std::nullopt;
            }
            if (it->is_string()) {
                return it->get <std::string>();
            }
            return it->dump();
        }

        const auto attr = node.xml.attribute(make_string(name).c_str());
        if (!attr) {
            return std::nullopt;
        }
        return attr.as_string();
    }

    bool has_attribute(const node_view& node, std::string_view name) {
        return attribute(node, name).has_value();
    }

    bool has_element(const node_view& node, std::string_view name) {
        if (node.is_json()) {
            return node.js && node.js->is_object() && node.js->contains(make_string(name));
        }
        return static_cast <bool>(node.xml.child(make_string(name).c_str()));
    }

    std::string text(const node_view& node) {
        if (node.is_json()) {
            if (node.js && node.js->is_string()) {
                return node.js->get <std::string>();
            }
            return {};
        }
        return node.xml.text().get();
    }

    std::string get_string(
        const node_view& node,
        std::string_view name,
        std::string_view default_value) {
        if (const auto value = attribute(node, name)) {
            return *value;
        }
        return make_string(default_value);
    }

    std::string require_string(const node_view& node, std::string_view name) {
        if (const auto value = attribute(node, name)) {
            return *value;
        }
        fail("missing attribute <" + make_string(name) + ">");
    }

    unsigned get_uint(const node_view& node, std::string_view name, unsigned default_value) {
        if (const auto value = attribute(node, name)) {
            return parse_integral <unsigned>(*value, name);
        }
        return default_value;
    }

    unsigned require_uint(const node_view& node, std::string_view name) {
        return parse_integral <unsigned>(require_string(node, name), name);
    }

    int get_int(const node_view& node, std::string_view name, int default_value) {
        if (const auto value = attribute(node, name)) {
            return parse_integral <int>(*value, name);
        }
        return default_value;
    }

    int require_int(const node_view& node, std::string_view name) {
        return parse_integral <int>(require_string(node, name), name);
    }

    double get_double(const node_view& node, std::string_view name, double default_value) {
        if (const auto value = attribute(node, name)) {
            return parse_floating <double>(*value, name);
        }
        return default_value;
    }

    bool parse_bool(std::string_view value, std::string_view name) {
        const auto clean = trim(value);
        if (clean == "true" || clean == "1") {
            return true;
        }
        if (clean == "false" || clean == "0") {
            return false;
        }
        fail("failed to parse boolean attribute <" + make_string(name) + ">");
    }

    bool get_bool(const node_view& node, std::string_view name, bool default_value) {
        if (const auto value = attribute(node, name)) {
            return parse_bool(*value, name);
        }
        return default_value;
    }

    namespace {
        [[nodiscard]] std::uint8_t parse_hex_byte(std::string_view value, std::size_t offset) {
            if (offset + 2 > value.size()) {
                fail("invalid color value <" + make_string(value) + ">");
            }
            unsigned parsed{};
            const auto part = value.substr(offset, 2);
            const auto [ptr, ec] = std::from_chars(part.data(), part.data() + part.size(), parsed, 16);
            if (ec != std::errc{} || ptr != part.data() + part.size()) {
                fail("invalid color component <" + make_string(part) + ">");
            }
            return static_cast <std::uint8_t>(parsed);
        }
    }

    sdlpp::color parse_color(std::string_view raw) {
        const auto value = trim(raw);
        if (value.empty()) {
            return {0, 0, 0, 255};
        }
        if (value.size() == 9 && value[0] == '#') {
            return {
                parse_hex_byte(value, 3),
                parse_hex_byte(value, 5),
                parse_hex_byte(value, 7),
                parse_hex_byte(value, 1)
            };
        }
        if (value.size() == 7 && value[0] == '#') {
            return {
                parse_hex_byte(value, 1),
                parse_hex_byte(value, 3),
                parse_hex_byte(value, 5),
                255
            };
        }
        if (value.size() == 6) {
            return {
                parse_hex_byte(value, 0),
                parse_hex_byte(value, 2),
                parse_hex_byte(value, 4),
                255
            };
        }
        fail("invalid color value <" + value + ">");
    }

    std::optional <sdlpp::color> parse_optional_color(const node_view& node, std::string_view name) {
        if (const auto value = attribute(node, name); value && !value->empty()) {
            return parse_color(*value);
        }
        return std::nullopt;
    }
}
