//
// Created by igor on 17/07/2026.
//

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <sdlpp/video/surface.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/sprites.hh>
#include <ke/resources/cell.hh>


namespace rs {
    struct tile_sheet_def {
        sdlpp::surface image;
        std::vector<neutrino::rect> source_rects;
        std::vector<neutrino::point> origins;
    };

    struct game_resources {
        std::map<std::string, neutrino::cpu_texture_atlas> backdrops;
        std::map<std::string, tile_sheet_def> tile_sheets;
        std::vector<ke_level> levels;
        // DIG sound banks (KE_MAIN.DIG + the per-screen tracks), keyed by name; raw bytes a
        // rs::dig_decoder plays from. See tab.md §8 / dig.md.
        std::map<std::string, std::vector<std::uint8_t>> audio;
    };

    std::optional<game_resources> parse(std::istream& is);
}