//
// Created by igor on 05/07/2026.
//

#pragma once

#include "world/tmx/node.hh"

namespace neutrino::world_tmx {
    [[nodiscard]] world_image parse_image(const node_view& node);
}
