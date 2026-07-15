//
// See ke_world.hh.
//

#include "resources/ke_world.hh"

#include <memory>
#include <optional>
#include <utility>

#include <failsafe/enforce.hh>

#include <sdlpp/video/blend_mode.hh>

#include "resources/ke_assets.hh"

namespace rs {
    namespace {
        // Duplicate a surface once into a shared, immutable image the world can hold.
        [[nodiscard]] std::shared_ptr <const sdlpp::surface> share_surface(
            const sdlpp::surface& surf, const char* what) {
            auto dup = surf.duplicate();
            ENFORCE(dup.has_value())("failed to duplicate surface for", what);
            return std::make_shared <const sdlpp::surface>(std::move(*dup));
        }
    } // namespace

    neutrino::world_tileset build_collection_tileset(
        std::string name, const tile_sheet_def& sheet, unsigned first_gid) {
        auto shared = share_surface(sheet.image, name.c_str());
        const auto sw = static_cast <unsigned>(shared->width());
        const auto sh = static_cast <unsigned>(shared->height());

        neutrino::world_tileset ts;
        ts.first_gid = first_gid;
        ts.name = std::move(name);
        ts.tile_count = static_cast <unsigned>(sheet.source_rects.size());
        // Nominal size (unused for collection tiles -- source_rect governs), kept non-zero.
        ts.tile_width = sheet.source_rects.empty() ? 1u : static_cast <unsigned>(sheet.source_rects[0].w);
        ts.tile_height = sheet.source_rects.empty() ? 1u : static_cast <unsigned>(sheet.source_rects[0].h);
        ts.tiles.reserve(sheet.source_rects.size());

        for (std::size_t i = 0; i < sheet.source_rects.size(); ++i) {
            neutrino::world_image img;
            img.source = neutrino::image_from_surface{shared, std::nullopt};
            img.width = sw;
            img.height = sh;

            neutrino::world_tile t;
            t.id = static_cast <neutrino::world_local_tile_id>(i);
            t.image = std::move(img);
            t.source_rect = sheet.source_rects[i];
            ts.tiles.push_back(std::move(t));
        }
        return ts;
    }

    std::shared_ptr <const sdlpp::surface> build_background(const game_resources& res,
                                                            std::size_t fill_block) {
        auto bg_e = sdlpp::surface::create_rgb(ke_screen_w, ke_screen_h,
                                               sdlpp::pixel_format_enum::RGBA8888);
        ENFORCE(bg_e.has_value())("failed to create background surface");
        sdlpp::surface bg = std::move(*bg_e);
        // Solid teal base (Mode-X colour fill, tab.md §6 / ke_dump render_level).
        (void) bg.fill(sdlpp::color{6, 34, 42, 255});

        // A blend-mode source blits alpha-over (BOB masks are binary 0/255), so
        // transparent texels leave the base showing through.
        auto blit_from = [&] (const sdlpp::surface& surface, const auto& fn) {
            auto src_e = surface.convert(sdlpp::pixel_format_enum::RGBA8888);
            ENFORCE(src_e.has_value())("failed to convert background source surface");
            sdlpp::surface src = std::move(*src_e);
            (void) src.set_blend_mode(sdlpp::blend_mode::blend);
            fn(src);
        };

        // Tiled KE_FILL block from y=16 across the screen.
        if (auto it = res.tile_sheets.find("ke_fill");
            it != res.tile_sheets.end() && fill_block < it->second.source_rects.size()) {
            const neutrino::rect fr = it->second.source_rects[fill_block];
            if (fr.w > 0 && fr.h > 0) {
                blit_from(it->second.image, [&] (sdlpp::surface& src) {
                    for (int y = 16; y < ke_screen_h; y += fr.h) {
                        for (int x = 0; x < ke_screen_w; x += fr.w) {
                            (void) src.blit_to(bg, std::optional{fr}, sdlpp::point <int>{x, y});
                        }
                    }
                });
            }
        }

        // KE_BORD pillars: block 0 = left edge, block 1 = right edge, both at y=8.
        if (auto it = res.tile_sheets.find("ke_bord");
            it != res.tile_sheets.end() && it->second.source_rects.size() > 1) {
            const neutrino::rect l = it->second.source_rects[0];
            const neutrino::rect r = it->second.source_rects[1];
            blit_from(it->second.image, [&] (sdlpp::surface& src) {
                (void) src.blit_to(bg, std::optional{l}, sdlpp::point <int>{0, ke_wall_top_y});
                (void) src.blit_to(bg, std::optional{r},
                                   sdlpp::point <int>{ke_screen_w - r.w, ke_wall_top_y});
            });
        }

        return std::make_shared <const sdlpp::surface>(std::move(bg));
    }

    neutrino::world build_world(const ke_level& level,
                                std::shared_ptr <const sdlpp::surface> background) {
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
