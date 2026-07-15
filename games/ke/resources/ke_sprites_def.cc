//
// Created by igor on 14/07/2026.
//

#include "ke_sprites_def.hh"

#include <memory>
#include <string>
#include <utility>

#include <failsafe/enforce.hh>
#include <failsafe/logger.hh>

#include "resources/ke_assets.hh"
#include "resources/ke_world.hh"

namespace rs {
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

    void define_sprites(const game_resources& gr) {
        define_blocks(gr);
        define_paddle(gr);
    }
}
