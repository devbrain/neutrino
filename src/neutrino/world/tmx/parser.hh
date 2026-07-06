//
// Created by igor on 05/07/2026.
//

#pragma once

#include <string_view>

#include <neutrino/world/tmx_loader.hh>
#include <neutrino/world/world.hh>

namespace neutrino::world_tmx {
    [[nodiscard]] world parse_document(
        std::string_view document,
        const tmx_path_resolver& resolver);
}
