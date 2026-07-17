//
// Created by igor on 17/07/2026.
//

#pragma once

#include <sdlpp/video/surface.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/sprites.hh>
#include "resources/ke_cell.hh"


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
    };

    std::optional<game_resources> parse(std::istream& is);
}