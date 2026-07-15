//
// Created by igor on 28/06/2026.
//

#pragma once

#include <iosfwd>
#include <vector>
#include <string>
#include <cstdint>
#include <map>
#include <optional>

#include <neutrino/video/sprite/cpu_texture_atlas.hh>
#include "resources/ke_cell.hh"

namespace rs {
    struct resource {
        std::string name;
        uint32_t offset;
        uint32_t size;
    };

    struct diagnostic {
        enum class severity_level {
            error,
            warning
        };

        severity_level severity;
        std::string message;
    };

    struct load_result {
        std::vector <resource> resources;
        std::vector <diagnostic> diagnostics;

        [[nodiscard]] bool has_errors() const {
            for (const auto& diag : diagnostics) {
                if (diag.severity == diagnostic::severity_level::error) {
                    return true;
                }
            }
            return false;
        }
    };

    load_result load_resource(std::istream& is);

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
