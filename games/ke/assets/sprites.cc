//
// Created by igor on 14/07/2026.
//

#include <ke/assets/sprites.hh>

#include <memory>
#include <string>
#include <utility>

#include <failsafe/enforce.hh>
#include <failsafe/logger.hh>

#include <ke/assets/registry.hh>

namespace rs {

    namespace {
        // Duplicate a surface once into a shared, immutable image the world can hold.
        [[nodiscard]] std::shared_ptr <const sdlpp::surface> share_surface(
            const sdlpp::surface& surf, const char* what) {
            auto dup = surf.duplicate();
            ENFORCE(dup.has_value())("failed to duplicate surface for", what);
            return std::make_shared <const sdlpp::surface>(std::move(*dup));
        }

        // A collection tileset from a decoded BOB sheet: one tile per frame (local id = frame
        // index), each a source_rect into a shared duplicate of the packed surface. A brick with
        // frame F gets gid = first_gid + F.
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
    }

    neutrino::sprite_def to_sprite_def(const tile_sheet_def& sheet) {
        auto dup = sheet.image.duplicate();
        ENFORCE(dup.has_value())("failed to duplicate sheet surface");
        auto shared = std::make_shared <const sdlpp::surface>(std::move(*dup));

        neutrino::sprite_def def;
        neutrino::world_image img;
        img.source = neutrino::image_from_surface{shared, std::nullopt};
        img.width = static_cast <unsigned>(shared->width());
        img.height = static_cast <unsigned>(shared->height());
        def.image = std::move(img);

        def.visuals.reserve(sheet.source_rects.size());
        for (std::size_t i = 0; i < sheet.source_rects.size(); ++i) {
            neutrino::sprite_visual_def v;
            v.name = std::to_string(i);
            v.src = sheet.source_rects[i];
            // The BOB per-frame offset is the pivot (keeps variable-size frames aligned).
            v.origin = i < sheet.origins.size() ? sheet.origins[i] : neutrino::point{0, 0};
            def.visuals.push_back(std::move(v));
        }
        return def;
    }

    namespace {
        // The brick tileset (the per-level background is built with each level's world).
        void define_blocks(const game_resources& gr) {
            const auto brick = gr.tile_sheets.find("ke_brick");
            ENFORCE(brick != gr.tile_sheets.end())("no ke_brick sheet");
            require_ke_assets().blocks = build_collection_tileset("ke_brick", brick->second, 1);
        }

        // The paddle sprite def (KE_RACK visuals with origins), built and leased from the cache.
        void define_paddle(const game_resources& gr) {
            const auto rack = gr.tile_sheets.find("ke_rack");
            if (rack == gr.tile_sheets.end()) {
                LOG_ERROR("ke: no ke_rack sheet -- paddle undefined");
                return;
            }
            ke_assets& a = require_ke_assets();
            a.paddle_def = to_sprite_def(rack->second); // clips (grow/shrink/transform) TODO
            a.paddle = a.cache.acquire(a.paddle_def);
        }
    } // namespace

    void define_sprites(game_resources& gr) {
        ke_assets& a = require_ke_assets();

        if (auto i = gr.tile_sheets.find("ke_bord"); i != gr.tile_sheets.end() && i->second.source_rects.size() > 1) {
            a.board = std::move(i->second);
            gr.tile_sheets.erase(i);
        } else {
            THROW_RUNTIME("ke_board is absent or corrupt");
        }

        if (auto i = gr.tile_sheets.find("ke_fill"); i != gr.tile_sheets.end() && i->second.source_rects.size() > 1) {
            a.fill_rects = std::move(i->second);
            gr.tile_sheets.erase(i);
        } else {
            THROW_RUNTIME("ke_board is absent or corrupt");
        }

        define_blocks(gr);
        define_paddle(gr);
    }
}
