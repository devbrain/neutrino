//
// Created by igor on 14/10/2026.
//

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <filesystem>

#include <nlohmann/json.hpp>
#include <luxor/util/expected.hh>
#include <luxor/resources/parser/cfg_parser.hh>

namespace luxor::rc {

    struct sl3_sample {
        std::string name;
        std::string file;
        float volume{1.0f};
        std::vector<property> properties;

        [[nodiscard]] nlohmann::json to_json() const;
    };

    struct sl3_stream {
        std::string name;
        std::string file;
        bool loop{true};
        std::vector<property> properties;

        [[nodiscard]] nlohmann::json to_json() const;
    };

    struct sl3_playlist {
        std::string name;
        std::string flags;
        std::vector<std::string> tracks;
        std::vector<property> properties;

        [[nodiscard]] nlohmann::json to_json() const;
    };

    struct sl3_file {
        std::unordered_map<std::string, sl3_sample> samples;
        std::unordered_map<std::string, sl3_stream> streams;
        std::unordered_map<std::string, sl3_playlist> playlists;

        std::vector<std::string> sample_order;
        std::vector<std::string> stream_order;
        std::vector<std::string> playlist_order;

        [[nodiscard]] bool has_sample(std::string_view name) const;
        [[nodiscard]] bool has_stream(std::string_view name) const;
        [[nodiscard]] bool has_playlist(std::string_view name) const;

        [[nodiscard]] const sl3_sample* find_sample(std::string_view name) const;
        [[nodiscard]] const sl3_stream* find_stream(std::string_view name) const;
        [[nodiscard]] const sl3_playlist* find_playlist(std::string_view name) const;

        [[nodiscard]] nlohmann::json to_json() const;
    };

    void to_json(nlohmann::json& j, const sl3_sample& sample);
    void to_json(nlohmann::json& j, const sl3_stream& stream);
    void to_json(nlohmann::json& j, const sl3_playlist& playlist);
    void to_json(nlohmann::json& j, const sl3_file& file);

    [[nodiscard]] expected<sl3_file, std::string> parse_sl3(const config_document& doc);
    [[nodiscard]] expected<sl3_file, std::string> parse_sl3(std::string_view content);
    [[nodiscard]] expected<sl3_file, std::string> parse_sl3_file(const std::filesystem::path& path);

} // namespace luxor::rc
