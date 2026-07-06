//
// Created by igor on 05/07/2026.
//

#pragma once

#include "world/tmx/component.hh"

namespace neutrino {
    class world;
}

namespace neutrino::world_tmx {
    struct group_context;
    struct node_view;
    void parse_layers(const node_view& node, world& out, const group_context* parent);
}
