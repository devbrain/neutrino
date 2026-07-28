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
            // top_left_origin pins the pivot to (0,0) so a sprite lines up with the physics
            // coordinate it is drawn at (KE places everything this way). Otherwise the BOB
            // per-frame offset is the pivot, aligning variable-size frames to a shared anchor.
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
        neutrino::sprite_def define_set(const game_resources& gr, const char* sheet_name,
                        neutrino::sprite_set_handle& set, bool top_left_origin = false) {
            const auto it = gr.tile_sheets.find(sheet_name);
            ENFORCE (it != gr.tile_sheets.end()) ("ke: no", sheet_name, "sheet -- set undefined");


            ke_assets& a = require_ke_assets();
            const neutrino::sprite_def def = to_sprite_def(it->second, top_left_origin);
            set = a.cache.acquire(def);
            return def;
        }

    } // namespace



    void define_sprites(const game_resources& gr) {
        ke_assets& a = require_ke_assets();

        // All sets place by top-left (pivot (0,0)) so a sprite lines up with the physics
        // coordinate the game draws it at: the paddle/brick collider top-left, or -- via the
        // centred draw in play_game_scene -- the ball collider centre. Keeping the BOB
        // per-frame offset as the pivot would shift each sprite off its body and the walls.
        define_set(gr, "ke_rack", a.paddle, /*top_left=*/true);
        define_set(gr, "ke_brick", a.bricks, /*top_left=*/true);
        auto spell_def = define_set(gr, "ke_spell", a.balls, /*top_left=*/true);
        define_set(gr, "ke_bord", a.board, /*top_left=*/true);
        define_set(gr, "ke_fill", a.fill, /*top_left=*/true);
    }
}
