//
// Created by igor on 05/07/2026.
//

#pragma once

#include "world/tmx/component.hh"

namespace neutrino::world_tmx {
    [[nodiscard]] world_object parse_object(const node_view& node);
    [[nodiscard]] world_object_layer parse_object_layer(const node_view& node, const group_context* parent);
}
