//
// Bridges decoded KE resources into neutrino's world/render systems:
//   * a decoded BOB sheet -> a collection tileset (build_collection_tileset);
//   * KE_FILL/KE_BORD -> a composed background surface (build_background);
//   * a decoded level -> a neutrino::world using the published ke_assets (build_world).
//

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include <sdlpp/video/surface.hh>

#include <neutrino/world/world.hh>
#include <neutrino/world/world_common.hh>
#include <neutrino/world/world_layers.hh>
#include <neutrino/world/world_tileset.hh>
#include <ke/resources/resources.hh>
#include <ke/resources/cell.hh>

namespace rs {
    // KE screen + playfield geometry (tab.md §3/§6, mirrored from ke_dump render_level).
    inline constexpr int ke_screen_w = 320;
    inline constexpr int ke_screen_h = 200;
    inline constexpr int ke_cell_w = 16;
    inline constexpr int ke_cell_h = 8;
    inline constexpr int ke_grid_x = 16;
    inline constexpr int ke_grid_y = 24;
    // KE_FILL block 6 is the level-entry fill tile (the engine cycles 6..46; the static
    // background locks to block 6).
    inline constexpr std::size_t ke_default_fill_block = 6;

    // KE_FILL blocks 6..46 are the per-level fill tiles; successive levels step through
    // them, so each level's background differs (mirrors the original engine cycling 6..46).
    inline constexpr std::size_t ke_fill_block_first = ke_default_fill_block; // 6
    inline constexpr std::size_t ke_fill_block_count = 41; // blocks 6..46

    // The KE_FILL block for a level, cycling through 6..46 (level 0 -> ke_default_fill_block).
    // build_background clamps an out-of-range block, so this stays safe regardless of how
    // many tiles the KE_FILL sheet actually holds.
    [[nodiscard]] inline std::size_t ke_fill_block_for_level(int level) {
        const auto l = static_cast <std::size_t>(level < 0 ? 0 : level);
        return ke_fill_block_first + (l % ke_fill_block_count);
    }

    // Stable id of the brick tile layer, so a world_compositor can slot the actor
    // render_layer right after it (actors draw above the bricks).
    inline constexpr neutrino::world_layer_id ke_bricks_layer_id = 2;

    [[nodiscard]] neutrino::world_tileset build_collection_tileset(
        std::string name, const tile_sheet_def& sheet, unsigned first_gid);

    struct world_geometry {
        int left_margin;
        int right_margin;
        int top_margin;
        int bottom_margin;
        neutrino::point score;
        neutrino::point lives;
        neutrino::point high_score;
        neutrino::point portal;
        neutrino::point paddle_start;
    };

    // Compose the per-level background as ke_dump does (render_level): a solid teal base,
    // a tiled KE_FILL block from y=16, and the two KE_BORD wall pillars. @p fill_block selects
    // the KE_FILL tile (block 6 default).
    [[nodiscard]] std::pair <std::shared_ptr <const sdlpp::surface>, world_geometry> build_background(
        const game_resources& res, std::size_t fill_block = ke_default_fill_block);

    // Assemble the static world for one level, back to front:
    //   [background image layer] [brick tile layer (id = ke_bricks_layer_id)].
    // The brick tileset comes from the published ke_assets; @p background is the level's
    // own composed surface (from build_background with its fill_block).
    // @pre set_ke_assets()/define_sprites() have run.
    [[nodiscard]] neutrino::world build_world(const ke_level& level,
                                              const std::shared_ptr <const sdlpp::surface>& background);
}
