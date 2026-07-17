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


}
