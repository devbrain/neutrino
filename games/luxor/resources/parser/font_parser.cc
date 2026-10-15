//
// Created by igor on 12/10/2026.
//

#include "luxor/resources/parser/font_parser.hh"

#include <fstream>
#include <sstream>
#include <algorithm>

namespace luxor::rc {

    bool font_file::has_glyph(char32_t codepoint) const {
        return find_glyph(codepoint) != nullptr;
    }

    bool font_file::has_glyph(char c) const {
        return find_glyph(c) != nullptr;
    }

    const font_glyph* font_file::find_glyph(char32_t codepoint) const {
        for (const auto& g : glyphs) {
            if (g.codepoint == codepoint) {
                return &g;
            }
        }
        return nullptr;
    }

    const font_glyph* font_file::find_glyph(char c) const {
        const char32_t cp = static_cast<char32_t>(static_cast<unsigned char>(c));
        return find_glyph(cp);
    }

    int font_file::calculate_text_width(std::string_view text, int spacing) const {
        int width = 0;
        bool first = true;
        for (char c : text) {
            const auto* g = find_glyph(c);
            int glyph_w = g ? g->width : (char_height / 2);
            if (!first) {
                width += spacing;
            }
            width += glyph_w;
            first = false;
        }
        return width;
    }

    nlohmann::json font_glyph::to_json() const {
        return {
            {"codepoint", static_cast<uint32_t>(codepoint)},
            {"character", character},
            {"x", x},
            {"y", y},
            {"width", width},
            {"height", height}
        };
    }

    void to_json(nlohmann::json& j, const font_glyph& glyph) {
        j = glyph.to_json();
    }

    nlohmann::json font_file::to_json() const {
        auto glyphs_arr = nlohmann::json::array();
        for (const auto& g : glyphs) {
            glyphs_arr.push_back(g.to_json());
        }
        return {
            {"diffuse_texture", diffuse_texture},
            {"alpha_texture", alpha_texture},
            {"has_alpha", has_alpha},
            {"char_height", char_height},
            {"rows", rows},
            {"glyph_count", glyph_count},
            {"glyphs", std::move(glyphs_arr)}
        };
    }

    void to_json(nlohmann::json& j, const font_file& font) {
        j = font.to_json();
    }

    namespace {
        char32_t decode_utf8(std::string_view s) {
            if (s.empty()) {
                return 0;
            }
            const auto b0 = static_cast<unsigned char>(s[0]);
            if ((b0 & 0x80) == 0) {
                return b0;
            }
            if ((b0 & 0xE0) == 0xC0 && s.size() >= 2) {
                return ((b0 & 0x1F) << 6) | (static_cast<unsigned char>(s[1]) & 0x3F);
            }
            if ((b0 & 0xF0) == 0xE0 && s.size() >= 3) {
                return ((b0 & 0x0F) << 12)
                     | ((static_cast<unsigned char>(s[1]) & 0x3F) << 6)
                     | (static_cast<unsigned char>(s[2]) & 0x3F);
            }
            if ((b0 & 0xF8) == 0xF0 && s.size() >= 4) {
                return ((b0 & 0x07) << 18)
                     | ((static_cast<unsigned char>(s[1]) & 0x3F) << 12)
                     | ((static_cast<unsigned char>(s[2]) & 0x3F) << 6)
                     | (static_cast<unsigned char>(s[3]) & 0x3F);
            }
            return b0;
        }
    } // namespace

    expected<font_file, std::string> parse_font(std::string_view content) {
        std::vector<std::string> lines;
        {
            std::string current;
            for (char c : content) {
                if (c == '\n') {
                    lines.push_back(std::move(current));
                    current.clear();
                } else if (c != '\r') {
                    current.push_back(c);
                }
            }
            if (!current.empty()) {
                lines.push_back(std::move(current));
            }
        }

        size_t pos = 0;
        auto next_line_trimmed = [&](std::string& out) -> bool {
            while (pos < lines.size()) {
                out = lines[pos++];
                while (!out.empty() && (out.back() == ' ' || out.back() == '\t')) {
                    out.pop_back();
                }
                if (out.empty() || out.rfind("//", 0) == 0) {
                    continue;
                }
                return true;
            }
            return false;
        };

        font_file font{};

        // Line 1: Diffuse Texture Path
        if (!next_line_trimmed(font.diffuse_texture)) {
            return unexpected("Unexpected EOF reading diffuse texture path");
        }
        std::replace(font.diffuse_texture.begin(), font.diffuse_texture.end(), '\\', '/');

        // Line 2: Alpha Texture Path
        if (!next_line_trimmed(font.alpha_texture)) {
            return unexpected("Unexpected EOF reading alpha texture path");
        }
        std::replace(font.alpha_texture.begin(), font.alpha_texture.end(), '\\', '/');
        font.has_alpha = (font.alpha_texture != "none" && !font.alpha_texture.empty());

        // Line 3: Character Height
        std::string line;
        if (!next_line_trimmed(line)) {
            return unexpected("Unexpected EOF reading font height");
        }
        {
            std::istringstream ss(line);
            if (!(ss >> font.char_height) || font.char_height <= 0) {
                return unexpected("Failed to parse font height from: " + line);
            }
        }

        // Line 4: Rows & Glyph Count
        if (!next_line_trimmed(line)) {
            return unexpected("Unexpected EOF reading glyph count");
        }
        {
            std::istringstream ss(line);
            if (!(ss >> font.rows >> font.glyph_count) || font.glyph_count < 0) {
                return unexpected("Failed to parse rows/glyph_count from: " + line);
            }
        }

        font.glyphs.reserve(static_cast<size_t>(font.glyph_count));

        for (int i = 0; i < font.glyph_count; ++i) {
            if (pos >= lines.size()) {
                return unexpected("Unexpected EOF reading glyph character at index " + std::to_string(i));
            }
            std::string char_line = lines[pos++];
            // A space character glyph line consists of a single space
            if (char_line.empty()) {
                char_line = " ";
            }

            font_glyph glyph{};
            glyph.character = char_line;
            glyph.codepoint = decode_utf8(char_line);
            glyph.height = font.char_height;

            if (pos >= lines.size()) {
                return unexpected("Unexpected EOF reading coordinates for glyph '" + char_line + "'");
            }
            std::string coord_line = lines[pos++];
            std::istringstream ss(coord_line);
            if (!(ss >> glyph.x >> glyph.y >> glyph.width)) {
                return unexpected("Failed to parse glyph coordinates from: " + coord_line);
            }

            font.glyphs.push_back(std::move(glyph));
        }

        return font;
    }

    expected<font_file, std::string> parse_font_file(const std::filesystem::path& path) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs.is_open()) {
            return unexpected("Failed to open font file: " + path.string());
        }
        std::stringstream ss;
        ss << ifs.rdbuf();
        return parse_font(ss.str());
    }

} // namespace luxor::rc
