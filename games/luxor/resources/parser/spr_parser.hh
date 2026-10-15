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
#include <neutrino/video/sprite/sprite_def.hh>
#include "luxor/util/expected.hh"

namespace luxor::rc {

    struct sprite_frame {
        int x{0};
        int y{0};
        int width{0};
        int height{0};
        // Normalized UV coordinates [0.0 .. 1.0]
        float u_min{0.0f};
        float v_min{0.0f};
        float u_max{0.0f};
        float v_max{0.0f};

        [[nodiscard]] nlohmann::json to_json() const;
    };

    struct sprite_anim {
        std::string name;
        std::vector<sprite_frame> frames;

        [[nodiscard]] nlohmann::json to_json() const;
    };

    enum class sprite_format {
        classic,     // ASCII line-delimited (Luxor 1, Luxor: Amun Rising, Luxor Mahjong)
        gvf_binary,  // GVF002 binary bytecode AST (Luxor 2, Luxor 2 HD)
        declarative  // Declarative block text format (Luxor HD, Luxor Evolved)
    };

    [[nodiscard]] std::string_view to_string(sprite_format fmt);
    void to_json(nlohmann::json& j, sprite_format fmt);

    struct sprite_file {
        sprite_format format{sprite_format::classic};
        std::string diffuse_texture;
        std::string alpha_texture;
        bool has_alpha{false};
        neutrino::sprite_grid grid{};
        std::vector<sprite_anim> anims;

        // Additional materials/textures (primarily for GVF/declarative format):
        std::vector<std::string> materials;

        [[nodiscard]] size_t total_frames() const;
        [[nodiscard]] const sprite_frame* get_frame(size_t anim_index, size_t frame_index) const;

        [[nodiscard]] nlohmann::json to_json() const;
    };

    void to_json(nlohmann::json& j, const sprite_frame& frame);
    void to_json(nlohmann::json& j, const sprite_anim& anim);
    void to_json(nlohmann::json& j, const sprite_file& file);

    [[nodiscard]] expected<sprite_file, std::string> parse_spr(std::string_view content);
    [[nodiscard]] expected<sprite_file, std::string> parse_spr(const uint8_t* data, size_t size);
    [[nodiscard]] expected<sprite_file, std::string> parse_spr_file(const std::filesystem::path& path);

} // namespace luxor::rc

namespace neutrino {
    void to_json(nlohmann::json& j, const sprite_grid& grid);
}

namespace nlohmann {
    template <>
    struct adl_serializer<neutrino::sprite_grid> {
        static void to_json(json& j, const neutrino::sprite_grid& g) {
            neutrino::to_json(j, g);
        }
    };
}


