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

    neutrino::sprite_def to_sprite_def(const tile_sheet_def& sheet, bool top_left_origin) {
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
            // Actors keep the BOB per-frame offset as the pivot (aligns variable-size animation
            // frames); backdrop tiles want top-left placement, so their pivot is (0,0).
            v.origin = top_left_origin || i >= sheet.origins.size()
                           ? neutrino::point{0, 0}
                           : sheet.origins[i];
            def.visuals.push_back(std::move(v));
        }
        return def;
    }

    namespace {
        // Build a leased set from a named BOB sheet. The set's visuals are named "0".."N-1"
        // (frame index); callers resolve them via ke_paddle_frame / ke_ball_frame. The
        // source def is transient -- the built set answers its own frame geometry, so
        // nothing keeps the def alive past the acquire.
        void define_set(const game_resources& gr, const char* sheet_name,
                        neutrino::sprite_set_handle& set, bool top_left_origin = false) {
            const auto it = gr.tile_sheets.find(sheet_name);
            if (it == gr.tile_sheets.end()) {
                LOG_ERROR("ke: no", sheet_name, "sheet -- set undefined");
                return;
            }
            ke_assets& a = require_ke_assets();
            const neutrino::sprite_def def = to_sprite_def(it->second, top_left_origin);
            set = a.cache.acquire(def);
        }
    } // namespace

    void define_sprites(const game_resources& gr) {
        ke_assets& a = require_ke_assets();

        // Actor sets keep the BOB pivot; the backdrop sets (walls / fill) place by top-left.
        define_set(gr, "ke_rack", a.paddle);
        define_set(gr, "ke_brick", a.bricks);
        define_set(gr, "ke_spell", a.balls);
        define_set(gr, "ke_bord", a.board, /*top_left=*/true);
        define_set(gr, "ke_fill", a.fill, /*top_left=*/true);
    }
}
