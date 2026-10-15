//
// Created by igor on 12/10/2026.
//

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <filesystem>
#include <cstdint>

#include <nlohmann/json.hpp>
#include "luxor/util/expected.hh"

namespace luxor::rc {

    // A single depth layer entry
    struct depth_layer {
        uint32_t index{0};
        std::string name;

        [[nodiscard]] nlohmann::json to_json() const;
    };

    // Table of depth layers as defined in depths.txt
    struct depth_table {
        static constexpr uint32_t MAX_LAYERS = 96;

        std::vector<std::string> layers;

        [[nodiscard]] size_t size() const noexcept { return layers.size(); }
        [[nodiscard]] bool empty() const noexcept { return layers.empty(); }

        [[nodiscard]] bool has(std::string_view name) const;
        [[nodiscard]] std::optional<uint32_t> get_index(std::string_view name) const;
        [[nodiscard]] std::optional<std::string_view> get_name(uint32_t index) const;
        [[nodiscard]] const std::string& operator[](size_t index) const { return layers[index]; }

        // Checks if name is "#parent" (case-insensitive)
        [[nodiscard]] static bool is_parent(std::string_view name) noexcept;

        [[nodiscard]] nlohmann::json to_json() const;
    };

    // Generic line-based string list (e.g. stage_names.txt)
    struct string_list {
        std::vector<std::string> items;

        [[nodiscard]] size_t size() const noexcept { return items.size(); }
        [[nodiscard]] bool empty() const noexcept { return items.empty(); }
        [[nodiscard]] const std::string& operator[](size_t index) const { return items[index]; }

        [[nodiscard]] bool has(std::string_view item, bool case_sensitive = false) const;
        [[nodiscard]] std::optional<size_t> find_index(std::string_view item, bool case_sensitive = false) const;

        [[nodiscard]] nlohmann::json to_json() const;
    };

    void to_json(nlohmann::json& j, const depth_layer& layer);
    void to_json(nlohmann::json& j, const depth_table& table);
    void to_json(nlohmann::json& j, const string_list& list);

    [[nodiscard]] expected<depth_table, std::string> parse_depths(std::string_view content);
    [[nodiscard]] expected<depth_table, std::string> parse_depths_file(const std::filesystem::path& path);

    [[nodiscard]] expected<string_list, std::string> parse_string_list(std::string_view content);
    [[nodiscard]] expected<string_list, std::string> parse_string_list_file(const std::filesystem::path& path);

} // namespace luxor::rc
