//
// Created by igor on 12/10/2026.
//

#include "luxor/resources/parser/spr_parser.hh"

#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>

namespace luxor::rc {

    size_t sprite_file::total_frames() const {
        size_t total = 0;
        for (const auto& a : anims) {
            total += a.frames.size();
        }
        return total;
    }

    const sprite_frame* sprite_file::get_frame(size_t anim_index, size_t frame_index) const {
        if (anim_index >= anims.size()) {
            return nullptr;
        }
        const auto& a = anims[anim_index];
        if (frame_index >= a.frames.size()) {
            return nullptr;
        }
        return &a.frames[frame_index];
    }

    std::string_view to_string(sprite_format fmt) {
        switch (fmt) {
            case sprite_format::classic: return "classic";
            case sprite_format::gvf_binary: return "gvf_binary";
            case sprite_format::declarative: return "declarative";
        }
        return "unknown";
    }

    void to_json(nlohmann::json& j, sprite_format fmt) {
        j = to_string(fmt);
    }

    nlohmann::json sprite_frame::to_json() const {
        return {
            {"x", x},
            {"y", y},
            {"width", width},
            {"height", height},
            {"u_min", u_min},
            {"v_min", v_min},
            {"u_max", u_max},
            {"v_max", v_max}
        };
    }

    void to_json(nlohmann::json& j, const sprite_frame& frame) {
        j = frame.to_json();
    }

    nlohmann::json sprite_anim::to_json() const {
        auto frames_arr = nlohmann::json::array();
        for (const auto& f : frames) {
            frames_arr.push_back(f.to_json());
        }
        return {
            {"name", name},
            {"frames", std::move(frames_arr)}
        };
    }

    void to_json(nlohmann::json& j, const sprite_anim& anim) {
        j = anim.to_json();
    }

    std::string_view to_string(neutrino::sprite_origin_rule rule) {
        switch (rule) {
            case neutrino::sprite_origin_rule::top_left: return "top_left";
            case neutrino::sprite_origin_rule::top_center: return "top_center";
            case neutrino::sprite_origin_rule::top_right: return "top_right";
            case neutrino::sprite_origin_rule::center_left: return "center_left";
            case neutrino::sprite_origin_rule::center: return "center";
            case neutrino::sprite_origin_rule::center_right: return "center_right";
            case neutrino::sprite_origin_rule::bottom_left: return "bottom_left";
            case neutrino::sprite_origin_rule::bottom_center: return "bottom_center";
            case neutrino::sprite_origin_rule::bottom_right: return "bottom_right";
        }
        return "unknown";
    }

} // namespace luxor::rc

namespace neutrino {
    void to_json(nlohmann::json& j, const sprite_grid& g) {
        j = {
            {"cell_w", g.cell_w},
            {"cell_h", g.cell_h},
            {"columns", g.columns},
            {"count", g.count},
            {"margin", g.margin},
            {"spacing", g.spacing},
            {"origin", std::string(luxor::rc::to_string(g.origin))}
        };
    }
}

namespace luxor::rc {

    nlohmann::json sprite_file::to_json() const {
        auto anims_arr = nlohmann::json::array();
        for (const auto& a : anims) {
            anims_arr.push_back(a.to_json());
        }
        return {
            {"format", std::string(to_string(format))},
            {"diffuse_texture", diffuse_texture},
            {"alpha_texture", alpha_texture},
            {"has_alpha", has_alpha},
            {"grid", grid},
            {"anims", std::move(anims_arr)},
            {"materials", materials}
        };
    }

    void to_json(nlohmann::json& j, const sprite_file& file) {
        j = file.to_json();
    }

    namespace {
        constexpr uint8_t GVF_MAGIC[8] = { 0x01, 'G', 'V', 'F', '0', '0', '2', 0x01 };

        bool is_gvf(const uint8_t* data, size_t size) {
            return (size >= 8 && std::memcmp(data, GVF_MAGIC, 8) == 0);
        }

        expected<sprite_file, std::string> parse_classic_spr(std::string_view content) {
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
            auto next_line = [&](std::string& out) -> bool {
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

            sprite_file spr{};
            spr.format = sprite_format::classic;

            // Line 1: Diffuse Texture Path
            if (!next_line(spr.diffuse_texture)) {
                return unexpected("Unexpected EOF reading diffuse texture path");
            }
            std::replace(spr.diffuse_texture.begin(), spr.diffuse_texture.end(), '\\', '/');

            // Line 2: Alpha Texture Path
            if (!next_line(spr.alpha_texture)) {
                return unexpected("Unexpected EOF reading alpha texture path");
            }
            std::replace(spr.alpha_texture.begin(), spr.alpha_texture.end(), '\\', '/');
            spr.has_alpha = (spr.alpha_texture != "none" && !spr.alpha_texture.empty());

            // Line 3: Frame Width & Height
            std::string line;
            if (!next_line(line)) {
                return unexpected("Unexpected EOF reading frame dimensions");
            }
            {
                std::istringstream ss(line);
                unsigned cell_w = 0;
                unsigned cell_h = 0;
                if (!(ss >> cell_w >> cell_h)) {
                    return unexpected("Failed to parse frame dimensions from: " + line);
                }
                spr.grid.cell_w = cell_w;
                spr.grid.cell_h = cell_h;
            }

            // Line 4: Anim Count
            if (!next_line(line)) {
                return unexpected("Unexpected EOF reading animation count");
            }
            int anim_count = 0;
            {
                std::istringstream ss(line);
                if (!(ss >> anim_count) || anim_count < 0) {
                    return unexpected("Failed to parse animation count from: " + line);
                }
            }

            spr.anims.reserve(static_cast<size_t>(anim_count));

            for (int a = 0; a < anim_count; ++a) {
                if (!next_line(line)) {
                    return unexpected("Unexpected EOF reading animation " + std::to_string(a) + " header");
                }
                int frame_count = 0;
                char mode_flag = 0;
                {
                    std::istringstream ss(line);
                    if (!(ss >> frame_count) || frame_count < 0) {
                        return unexpected("Failed to parse frame count for animation " + std::to_string(a));
                    }
                    ss >> mode_flag;
                }

                sprite_anim anim;
                anim.frames.reserve(static_cast<size_t>(frame_count));

                const bool is_explicit = (mode_flag == 'X' || mode_flag == 'x');

                if (!is_explicit) {
                    if (!next_line(line)) {
                        return unexpected("Unexpected EOF reading coordinates for animation " + std::to_string(a));
                    }
                    int start_x = 0;
                    int start_y = 0;
                    {
                        std::istringstream ss(line);
                        if (!(ss >> start_x >> start_y)) {
                            return unexpected("Failed to parse start coordinates from: " + line);
                        }
                    }

                    for (int f = 0; f < frame_count; ++f) {
                        sprite_frame frame{};
                        frame.x = start_x + (f * static_cast<int>(spr.grid.cell_w));
                        frame.y = start_y;
                        frame.width = static_cast<int>(spr.grid.cell_w);
                        frame.height = static_cast<int>(spr.grid.cell_h);
                        anim.frames.push_back(frame);
                    }
                } else {
                    for (int f = 0; f < frame_count; ++f) {
                        if (!next_line(line)) {
                            return unexpected("Unexpected EOF reading explicit frame " + std::to_string(f) + " of animation " + std::to_string(a));
                        }
                        sprite_frame frame{};
                        frame.width = static_cast<int>(spr.grid.cell_w);
                        frame.height = static_cast<int>(spr.grid.cell_h);
                        std::istringstream ss(line);
                        if (!(ss >> frame.x >> frame.y)) {
                            return unexpected("Failed to parse explicit coordinates from: " + line);
                        }
                        anim.frames.push_back(frame);
                    }
                }

                spr.anims.push_back(std::move(anim));
            }

            return spr;
        }

        expected<sprite_file, std::string> parse_gvf_spr(const uint8_t* data, size_t size) {
            sprite_file spr{};
            spr.format = sprite_format::gvf_binary;

            size_t pos = 8;
            if (pos + 4 > size) {
                return unexpected("Unexpected EOF reading string table count");
            }

            uint32_t num_strings = 0;
            std::memcpy(&num_strings, data + pos, 4);
            pos += 4;

            std::vector<std::string> strings;
            strings.reserve(num_strings);

            for (uint32_t s = 0; s < num_strings; ++s) {
                if (pos + 4 > size) {
                    return unexpected("Unexpected EOF reading string length");
                }
                uint32_t slen = 0;
                std::memcpy(&slen, data + pos, 4);
                pos += 4;

                if (pos + slen > size) {
                    return unexpected("Unexpected EOF reading string data");
                }

                strings.emplace_back(reinterpret_cast<const char*>(data + pos), slen);
                pos += slen;
            }

            // 8-byte separator
            if (pos + 8 > size) {
                return unexpected("Unexpected EOF before bytecode payload");
            }
            pos += 8;

            std::string current_block;
            std::string current_anim_name;

            while (pos < size) {
                uint8_t op = data[pos++];

                if (op == 0x11) {
                    if (pos + 8 > size) {
                        return unexpected("Truncated block header (0x11)");
                    }
                    int32_t name_id = 0;
                    int32_t type_id = 0;
                    std::memcpy(&name_id, data + pos, 4);
                    std::memcpy(&type_id, data + pos + 4, 4);
                    pos += 8;

                    std::string name_str = (name_id >= 0 && static_cast<size_t>(name_id) < strings.size())
                                           ? strings[name_id] : "";
                    std::string type_str = (type_id >= 0 && static_cast<size_t>(type_id) < strings.size())
                                           ? strings[type_id] : "";

                    if (type_str == "Anim") {
                        current_anim_name = name_str;
                        sprite_anim anim;
                        anim.name = current_anim_name;
                        spr.anims.push_back(std::move(anim));
                    }
                    current_block = (name_str.empty()) ? type_str : name_str;
                } else if (op == 0x12) {
                    current_block.clear();
                } else if (op == 0x13) {
                    if (pos + 6 > size) {
                        return unexpected("Truncated property (0x13)");
                    }
                    int32_t key_id = 0;
                    std::memcpy(&key_id, data + pos, 4);
                    pos += 4;

                    uint8_t val_type = data[pos++];
                    uint8_t val_subtype = data[pos++];

                    std::string key_str = (key_id >= 0 && static_cast<size_t>(key_id) < strings.size())
                                          ? strings[key_id] : "";

                    if (val_type == 8 && val_subtype == 1) {
                        // String value
                        if (pos + 4 > size) {
                            return unexpected("Truncated string value length");
                        }
                        uint32_t vlen = 0;
                        std::memcpy(&vlen, data + pos, 4);
                        pos += 4;
                        if (pos + vlen > size) {
                            return unexpected("Truncated string value payload");
                        }
                        std::string val(reinterpret_cast<const char*>(data + pos), vlen);
                        pos += vlen;

                        if (key_str.rfind("Material_", 0) == 0 || key_str == "Material") {
                            std::string mat = val;
                            if (!mat.empty() && mat[0] == '~') {
                                mat.erase(0, 1);
                            }
                            std::replace(mat.begin(), mat.end(), '\\', '/');
                            spr.materials.push_back(mat);
                            if (spr.diffuse_texture.empty()) {
                                auto star = mat.find('*');
                                if (star != std::string::npos) {
                                    spr.diffuse_texture = mat.substr(0, star);
                                    spr.alpha_texture = mat.substr(star + 1);
                                    spr.has_alpha = true;
                                } else {
                                    spr.diffuse_texture = mat;
                                }
                            }
                        }
                    } else if (val_type == 2 && val_subtype == 1) {
                        pos += 4; // int32
                    } else if (val_type == 0 && val_subtype == 1) {
                        pos += 1; // uint8
                    } else if (val_type == 3 && val_subtype == 4) {
                        // float[4] UV rect: [u_min, v_min, u_max, v_max]
                        if (pos + 16 > size) {
                            return unexpected("Truncated float[4] Rect property");
                        }
                        float uv[4];
                        std::memcpy(uv, data + pos, 16);
                        pos += 16;

                        if (key_str == "Rect" && !spr.anims.empty()) {
                            sprite_frame frame{};
                            frame.u_min = uv[0];
                            frame.v_min = uv[1];
                            frame.u_max = uv[2];
                            frame.v_max = uv[3];
                            spr.anims.back().frames.push_back(frame);
                        }
                    } else {
                        // Skip unknown value types
                        return unexpected("Unknown GVF property value type: (" + std::to_string(val_type) + ", " + std::to_string(val_subtype) + ")");
                    }
                }
            }

            return spr;
        }
    } // namespace

    expected<sprite_file, std::string> parse_spr(std::string_view content) {
        return parse_spr(reinterpret_cast<const uint8_t*>(content.data()), content.size());
    }

    expected<sprite_file, std::string> parse_spr(const uint8_t* data, size_t size) {
        if (is_gvf(data, size)) {
            return parse_gvf_spr(data, size);
        }
        std::string_view text(reinterpret_cast<const char*>(data), size);
        return parse_classic_spr(text);
    }

    expected<sprite_file, std::string> parse_spr_file(const std::filesystem::path& path) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs.is_open()) {
            return unexpected("Failed to open sprite file: " + path.string());
        }
        ifs.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(ifs.tellg());
        ifs.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(size);
        if (size > 0) {
            ifs.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(size));
        }

        return parse_spr(buffer.data(), buffer.size());
    }

} // namespace luxor::rc
