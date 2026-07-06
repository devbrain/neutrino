//
// Created by igor on 05/07/2026.
//

#pragma once

#include "world/tmx/node.hh"

#include <optional>

namespace neutrino::world_tmx {
    struct group_context {
        std::optional <double> offset_x;
        std::optional <double> offset_y;
        std::optional <float> opacity;
        std::optional <bool> visible;
        std::optional <float> parallax_x;
        std::optional <float> parallax_y;
        std::optional <sdlpp::color> tint;
        world_component properties;
    };

    void parse_properties(
        world_component& out,
        const node_view& node,
        const world_component* inherited = nullptr);
    void parse_local_properties(world_component& out, const node_view& node);
    [[nodiscard]] group_context parse_group_context(const node_view& node, const group_context* parent);
    [[nodiscard]] world_layer_header parse_layer_header(const node_view& node, const group_context* parent);
}
