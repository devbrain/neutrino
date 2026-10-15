//
// Created by igor on 12/10/2026.
//

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <cstdint>

#include <nlohmann/json.hpp>
#include "luxor/util/expected.hh"

namespace luxor::rc {

    struct font_glyph {
        char32_t codepoint{0};
        std::string character;
        int x{0};
        int y{0};
        int width{0};
        int height{0};

        [[nodiscard]] nlohmann::json to_json() const;
    };

    struct font_file {
        std::string diffuse_texture;
        std::string alpha_texture;
        bool has_alpha{false};
        int char_height{0};
        int rows{0};
        int glyph_count{0};
        std::vector<font_glyph> glyphs;

        [[nodiscard]] bool has_glyph(char32_t codepoint) const;
        [[nodiscard]] bool has_glyph(char c) const;
        [[nodiscard]] const font_glyph* find_glyph(char32_t codepoint) const;
        [[nodiscard]] const font_glyph* find_glyph(char c) const;
        [[nodiscard]] int calculate_text_width(std::string_view text, int spacing = 0) const;

        [[nodiscard]] nlohmann::json to_json() const;
    };

    void to_json(nlohmann::json& j, const font_glyph& glyph);
    void to_json(nlohmann::json& j, const font_file& font);

    [[nodiscard]] expected<font_file, std::string> parse_font(std::string_view content);
    [[nodiscard]] expected<font_file, std::string> parse_font_file(const std::filesystem::path& path);

} // namespace luxor::rc


