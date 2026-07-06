//
// Created by igor on 05/07/2026.
//

#pragma once

#include "world/tmx/node.hh"
#include "world/tmx/parser.hh"

namespace neutrino::world_tmx {
    [[nodiscard]] world parse_world(const node_view& node, const tmx_path_resolver& resolver);
}
