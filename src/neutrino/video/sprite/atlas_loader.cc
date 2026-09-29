//
// Created by igor on 12/07/2026.
//

#include <neutrino/video/sprite/atlas_loader.hh>
#include <neutrino/video/sprite/sprite_def_builder.hh>

namespace neutrino {
    sprite_def load_aseprite_atlas(std::string_view text) {
        return sprite_def_builder::from_aseprite_json(text).build();
    }

    sprite_def load_aseprite_atlas(std::string_view text, std::vector <std::uint8_t> image_bytes) {
        return sprite_def_builder::from_aseprite_json(text, std::move(image_bytes)).build();
    }

    sprite_def load_aseprite_atlas(std::string_view text, std::span <const std::uint8_t> image_bytes) {
        return sprite_def_builder::from_aseprite_json(text, image_bytes).build();
    }

    sprite_def load_aseprite_atlas(std::string_view text, const sprite_resource_resolver& resolver) {
        return sprite_def_builder::from_aseprite_json(text, resolver).build();
    }
}
