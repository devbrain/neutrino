//
// The playfield backdrop: the KE_BORD wall + KE_FILL score/fill sprite sets composed into a
// static per-level texture (compose_backdrop, presentation) and the playfield geometry the
// domain reads (compute_playfield_geometry). Both work off the built sprite sets -- no raw
// sheets, no manual surface blits.
//

#pragma once

#include <cstddef>
#include <optional>

#include <neutrino/video/geometry_types.hh>            // neutrino::point / dim
#include <neutrino/video/render_texture.hh>            // the composed backdrop target
#include <neutrino/video/sprite/sprite_cache.hh>       // sprite_set_handle

namespace rs {
    // KE screen + playfield geometry (tab.md §3/§6, mirrored from ke_dump render_level).
    inline constexpr int ke_screen_w = 320;
    inline constexpr int ke_screen_h = 200;
    inline constexpr int ke_cell_w = 16;
    inline constexpr int ke_cell_h = 8;
    inline constexpr int ke_grid_x = 16;
    inline constexpr int ke_grid_y = 24;
    inline constexpr int ke_paddle_bottom_y = 190; // the paddle's bottom edge rides this row
    // KE_FILL fixed HUD blocks (blocks 6..46 are the per-level fill tiles).
    inline constexpr std::size_t ke_fill_score_left  = 0; // left score dummy
    inline constexpr std::size_t ke_fill_score_mid   = 1; // centre score dummy
    inline constexpr std::size_t ke_fill_score_right = 2; // right score dummy
    inline constexpr std::size_t ke_fill_bar         = 5; // horizontal bar under the score row

    // KE_BORD wall pillars.
    inline constexpr std::size_t ke_bord_left  = 0;
    inline constexpr std::size_t ke_bord_right = 1;

    // KE_FILL block 6 is the level-entry fill tile (the engine cycles 6..46; the static
    // background locks to block 6).
    inline constexpr std::size_t ke_default_fill_block = 6;

    // KE_FILL blocks 6..46 are the per-level fill tiles; successive levels step through
    // them, so each level's background differs (mirrors the original engine cycling 6..46).
    inline constexpr std::size_t ke_fill_block_first = ke_default_fill_block; // 6
    inline constexpr std::size_t ke_fill_block_count = 41; // blocks 6..46

    // The KE_FILL block for a level, cycling through 6..46 (level 0 -> ke_default_fill_block).
    // compose_backdrop clamps an out-of-range block, so this stays safe regardless of how
    // many tiles the KE_FILL sheet actually holds.
    [[nodiscard]] inline std::size_t ke_fill_block_for_level(int level) {
        const auto l = static_cast <std::size_t>(level < 0 ? 0 : level);
        return ke_fill_block_first + (l % ke_fill_block_count);
    }

    // The playfield extent + paddle start, derived from the backdrop layout. The domain reads
    // this (bounds for physics, the paddle's start); it carries no pixels.
    struct playfield_geometry {
        int left_margin;
        int right_margin;
        int top_margin;
        int bottom_margin;
        neutrino::point paddle_start;
    };

    // The playfield geometry for the current level: margins from the KE_BORD pillars / KE_FILL
    // bar (read as frame rects off @p board / @p fill), and the paddle start centred to the
    // widest paddle form (@p paddle). Level-invariant (the per-level fill tile does not move
    // the walls).
    [[nodiscard]] playfield_geometry compute_playfield_geometry(
        const neutrino::sprite_set_handle& board,
        const neutrino::sprite_set_handle& fill,
        const neutrino::sprite_set_handle& paddle);

    // Compose the per-level backdrop as ke_dump does (render_level): a teal base, a tiled
    // KE_FILL block from y=16, the two KE_BORD wall pillars, and the score bar/dummies -- all
    // drawn through a sprite_batch into an offscreen texture. @p fill_block selects the KE_FILL
    // tile (block 6 default). Returns nullopt if the render target cannot be created.
    [[nodiscard]] std::optional <neutrino::render_texture> compose_backdrop(
        const neutrino::sprite_set_handle& board,
        const neutrino::sprite_set_handle& fill,
        std::size_t fill_block = ke_default_fill_block);
}
