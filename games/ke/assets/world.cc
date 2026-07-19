//
// See ke_world.hh.
//

#include <ke/assets/world.hh>

#include <memory>
#include <optional>
#include <utility>

#include <failsafe/enforce.hh>

#include <sdlpp/video/blend_mode.hh>

#include <ke/assets/registry.hh>

namespace rs {
    std::pair <std::shared_ptr <const sdlpp::surface>, world_geometry> build_background(const game_resources& res,
        std::size_t fill_block) {
        auto bg_e = sdlpp::surface::create_rgb(ke_screen_w, ke_screen_h,
                                               sdlpp::pixel_format_enum::RGBA8888);
        ENFORCE(bg_e.has_value())("failed to create background surface");
        sdlpp::surface bg = std::move(*bg_e);
        // Solid teal base (Mode-X colour fill, tab.md §6 / ke_dump render_level).
        (void)bg.fill(sdlpp::color{6, 34, 42, 255});

        // A blend-mode source blits alpha-over (BOB masks are binary 0/255), so
        // transparent texels leave the base showing through.
        auto blit_from = [&](const tile_sheet_def& tld, const auto& fn) {
            auto src_e = tld.image.convert(sdlpp::pixel_format_enum::RGBA8888);
            ENFORCE(src_e.has_value())("failed to convert background source surface");
            sdlpp::surface src = std::move(*src_e);
            (void)src.set_blend_mode(sdlpp::blend_mode::blend);
            fn(src);
        };

        world_geometry geometry;
        const ke_assets& assets = require_ke_assets();

        // score dummies
        int h = 0;
        blit_from(assets.fill_rects, [&](const sdlpp::surface& src) {
            const neutrino::rect l = assets.fill_rects.source_rects[0];
            const neutrino::rect c = assets.fill_rects.source_rects[1];
            const neutrino::rect r = assets.fill_rects.source_rects[2];
            auto total_w = l.w + c.w + r.w;
            auto gap = (ke_screen_w - total_w) / 2;
            h = l.h;

            geometry.score = {0, 0};
            geometry.lives = {l.w + gap, 0};
            geometry.high_score = {l.w + gap + c.w + gap, 0};

            (void)src.blit_to(bg, std::optional{l}, sdlpp::point <int>{0, 0});
            (void)src.blit_to(bg, std::optional{c}, sdlpp::point <int>{l.w + gap, 0});
            (void)src.blit_to(bg, std::optional{r}, sdlpp::point <int>{l.w + gap + c.w + gap, 0});
        });

        // Tiled KE_FILL block from y=16 across the screen.
        {
            if (const neutrino::rect fr = assets.fill_rects.source_rects[fill_block]; fr.w > 0 && fr.h > 0) {
                blit_from(assets.fill_rects, [&](sdlpp::surface& src) {
                    for (int y = 16; y < ke_screen_h; y += fr.h) {
                        for (int x = 0; x < ke_screen_w; x += fr.w) {
                            (void)src.blit_to(bg, std::optional{fr}, sdlpp::point <int>{x, y});
                        }
                    }
                });
            }
        }

        // KE_BORD pillars: block 0 = left edge, block 1 = right edge, both at y=8.
        {
            const neutrino::rect l = assets.board.source_rects[0];
            const neutrino::rect r = assets.board.source_rects[1];
            blit_from(assets.board, [&](const sdlpp::surface& src) {
                (void)src.blit_to(bg, std::optional{l}, sdlpp::point <int>{0, h});
                (void)src.blit_to(bg, std::optional{r},
                                  sdlpp::point <int>{ke_screen_w - r.w, h});
            });
            geometry.left_margin = l.w;
            geometry.right_margin = ke_screen_w - r.w;
            geometry.bottom_margin = ke_screen_h;
        }
        // KE_FILL horizontal bar and score dummies
        {
            blit_from(assets.fill_rects, [&](const sdlpp::surface& src) {
                const neutrino::rect r = assets.fill_rects.source_rects[5];
                (void)src.blit_to(bg, std::optional{r}, sdlpp::point <int>{16, h});
                geometry.top_margin = h + r.h;
            });
        }
        {
            int max_w = 0;
            int max_h = 0;
            int k = 0;
            for (const auto& v : assets.paddle_def.visuals) {
                if (k > 6) {
                    max_w = std::max(max_w, v.src.w); // v.src is a neutrino::rect -> .w/.h
                    max_h = std::max(max_h, v.src.h);
                }
                k++;
            }

            // Start centred and bottom-aligned. (x = ke_screen_w put the pivot off the
            // right edge, which is why the paddle was invisible.)
            geometry.paddle_start = {(ke_screen_w - max_w) / 2, ke_screen_h - max_h};
        }

        return {std::make_shared <const sdlpp::surface>(std::move(bg)), geometry};
    }

    neutrino::world build_world(const ke_level& level,
                                const std::shared_ptr <const sdlpp::surface>& background) {
        const ke_assets& assets = require_ke_assets();

        neutrino::world w;
        w.set_orientation(neutrino::world_orientation::orthogonal);
        w.set_size(static_cast <unsigned>(ke_level::cols), static_cast <unsigned>(ke_level::rows));
        w.set_tile_size(ke_cell_w, ke_cell_h);
        w.add_tileset(assets.blocks); // copies; the tiles' surface shared_ptr is cheap to copy

        // 1. Background: this level's composed surface as a single full-screen image layer.
        if (background) {
            neutrino::world_image img;
            img.source = neutrino::image_from_surface{background, std::nullopt};
            img.width = static_cast <unsigned>(background->width());
            img.height = static_cast <unsigned>(background->height());

            neutrino::world_image_layer bg;
            bg.id = 1;
            bg.name = "background";
            bg.image = std::move(img);
            bg.offset = {0.0f, 0.0f};
            w.add_layer(std::move(bg));
        }

        // 2. Bricks: an 18x16 tile layer, offset to the grid origin. gid = the brick
        //    tileset's first_gid + (tile_id - 1); empty cells (tile_id 0) stay gid 0.
        const unsigned brick_gid0 = assets.blocks.first_gid;

        neutrino::world_tile_layer bricks;
        bricks.id = ke_bricks_layer_id;
        bricks.name = "bricks";
        bricks.width = static_cast <unsigned>(ke_level::cols);
        bricks.height = static_cast <unsigned>(ke_level::rows);
        bricks.offset = {static_cast <float>(ke_grid_x), static_cast <float>(ke_grid_y)};
        bricks.cells.resize(static_cast <std::size_t>(ke_level::cols) * ke_level::rows);
        for (int y = 0; y < ke_level::rows; ++y) {
            for (int x = 0; x < ke_level::cols; ++x) {
                const auto& c = level.at(x, y);
                const auto idx = static_cast <std::size_t>(y) * ke_level::cols + x;
                bricks.cells[idx].gid =
                    c.tile_id == 0 ? 0u : brick_gid0 + static_cast <unsigned>(c.tile_id) - 1u;
            }
        }
        w.add_layer(std::move(bricks));

        return w;
    }
}
