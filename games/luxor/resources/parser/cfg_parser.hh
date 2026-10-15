//
// Created by igor on 11/10/2026.
//

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <variant>

#include <nlohmann/json.hpp>
#include "luxor/util/expected.hh"

namespace luxor::rc {

    using value = std::variant<std::string, bool, double>;

    struct property {
        std::string key;
        std::vector<value> values;

        [[nodiscard]] nlohmann::json to_json() const;
    };

    struct section {
        std::string type;
        std::string name;
        std::string target;
        std::vector<property> properties;

        [[nodiscard]] bool has(std::string_view key) const;
        [[nodiscard]] const property* find(std::string_view key) const;

        [[nodiscard]] std::string_view get_string(std::string_view key, std::string_view def = "") const;
        [[nodiscard]] bool get_bool(std::string_view key, bool def = false) const;
        [[nodiscard]] double get_number(std::string_view key, double def = 0.0) const;
        [[nodiscard]] int get_int(std::string_view key, int def = 0) const;
        [[nodiscard]] std::vector<double> get_numbers(std::string_view key) const;
        [[nodiscard]] const std::vector<value>* get_values(std::string_view key) const;

        [[nodiscard]] nlohmann::json to_json() const;
    };

    struct config_document {
        std::vector<property> properties;
        std::vector<section> sections;

        [[nodiscard]] bool has(std::string_view key) const;
        [[nodiscard]] const property* find(std::string_view key) const;

        [[nodiscard]] std::string_view get_string(std::string_view key, std::string_view def = "") const;
        [[nodiscard]] bool get_bool(std::string_view key, bool def = false) const;
        [[nodiscard]] double get_number(std::string_view key, double def = 0.0) const;
        [[nodiscard]] int get_int(std::string_view key, int def = 0) const;
        [[nodiscard]] std::vector<double> get_numbers(std::string_view key) const;
        [[nodiscard]] const std::vector<value>* get_values(std::string_view key) const;

        [[nodiscard]] bool has_section(std::string_view name) const;
        [[nodiscard]] bool has_section(std::string_view type, std::string_view name) const;
        [[nodiscard]] const section* find_section(std::string_view name) const;
        [[nodiscard]] const section* find_section(std::string_view type, std::string_view name) const;
        [[nodiscard]] std::vector<const section*> find_sections_by_type(std::string_view type) const;

        // Section property access (3-arg overloads)
        [[nodiscard]] std::string_view get_string(std::string_view section_name, std::string_view key, std::string_view def) const;
        [[nodiscard]] bool get_bool(std::string_view section_name, std::string_view key, bool def) const;
        [[nodiscard]] double get_number(std::string_view section_name, std::string_view key, double def) const;
        [[nodiscard]] int get_int(std::string_view section_name, std::string_view key, int def) const;

        // Section property access with default fallback values
        [[nodiscard]] std::string_view get_section_string(std::string_view section_name, std::string_view key, std::string_view def = "") const;
        [[nodiscard]] bool get_section_bool(std::string_view section_name, std::string_view key, bool def = false) const;
        [[nodiscard]] double get_section_number(std::string_view section_name, std::string_view key, double def = 0.0) const;
        [[nodiscard]] int get_section_int(std::string_view section_name, std::string_view key, int def = 0) const;

        [[nodiscard]] nlohmann::json to_json() const;
    };

    void to_json(nlohmann::json& j, const value& val);
    void to_json(nlohmann::json& j, const property& prop);
    void to_json(nlohmann::json& j, const section& sec);
    void to_json(nlohmann::json& j, const config_document& doc);

    using cfg_document = config_document;
    using level_file = config_document;

    [[nodiscard]] expected<config_document, std::string> parse_config(std::string_view content);
    [[nodiscard]] expected<config_document, std::string> parse_config_file(const std::filesystem::path& path);

    inline expected<cfg_document, std::string> parse_cfg(std::string_view content) {
        return parse_config(content);
    }
    inline expected<cfg_document, std::string> parse_cfg_file(const std::filesystem::path& path) {
        return parse_config_file(path);
    }

    inline expected<level_file, std::string> parse_level(std::string_view content) {
        return parse_config(content);
    }
    inline expected<level_file, std::string> parse_level_file(const std::filesystem::path& path) {
        return parse_config_file(path);
    }

    inline expected<config_document, std::string> parse_ini(std::string_view content) {
        return parse_config(content);
    }
    inline expected<config_document, std::string> parse_ini_file(const std::filesystem::path& path) {
        return parse_config_file(path);
    }

    inline expected<config_document, std::string> parse_psys(std::string_view content) {
        return parse_config(content);
    }
    inline expected<config_document, std::string> parse_psys_file(const std::filesystem::path& path) {
        return parse_config_file(path);
    }

} // namespace luxor::rc

namespace nlohmann {
    template <>
    struct adl_serializer<luxor::rc::value> {
        static void to_json(json& j, const luxor::rc::value& val) {
            luxor::rc::to_json(j, val);
        }
    };
} // namespace nlohmann

