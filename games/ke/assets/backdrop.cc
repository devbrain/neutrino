//
// See backdrop.hh.
//

#include <ke/assets/backdrop.hh>

#include <failsafe/enforce.hh>

#include <neutrino/video/world/sprite_batch.hh>
#include <neutrino/video/geometry_types.hh> // world_point
#include <ke/assets/sprites.hh>             // ke_paddle_frame (active form)

namespace rs {
    // Every frame looked up here is a fixed KE_BORD / KE_FILL / KE_RACK block the playfield is
    // built around -- a missing one means the wrong sheet was loaded, not that the backdrop has
    // no border. require_* aborts naming the block; the old value_or(rect{}) silently produced a
    // zero-size margin, i.e. a playfield whose walls sat in the wrong place.
    playfield_geometry compute_playfield_geometry(const neutrino::sprite_set_handle& board,
                                                  const neutrino::sprite_set_handle& fill,
                                                  const neutrino::sprite_set_handle& paddle) {
        // The score row sits at the top; the pillars and bar start just below it.
        const int score_h = fill.require_frame_rect(ke_fill_score_left).h;
        const neutrino::rect l = board.require_frame_rect(ke_bord_left);
        const neutrino::rect r = board.require_frame_rect(ke_bord_right);
        const neutrino::rect bar = fill.require_frame_rect(ke_fill_bar);

        playfield_geometry geo{};
        geo.left_margin = l.w;
        geo.right_margin = ke_screen_w - r.w;
        geo.top_margin = score_h + bar.h;
        geo.bottom_margin = ke_screen_h;

        // Centre + bottom-align the paddle to its ACTIVE (default) form. game_mechanics builds
        // the collider from this exact frame and launches the ball off it, so using the
        // bounding cell of all forms would start the paddle off-centre and above the paddle row.
        const neutrino::rect form = paddle.require_frame_rect(
            rs::ke_paddle_frame(rs::ke_paddle_state::simple, rs::ke_paddle_default_size));
        geo.paddle_start = {(ke_screen_w - form.w) / 2, ke_paddle_bottom_y - form.h};
        return geo;
    }

    std::optional <neutrino::render_texture> compose_backdrop(const neutrino::sprite_set_handle& board,
                                                              const neutrino::sprite_set_handle& fill,
                                                              std::size_t fill_block) {
        auto target = neutrino::render_texture::create(neutrino::dim{ke_screen_w, ke_screen_h});
        if (!target) {
            return std::nullopt;
        }

        // Teal base (Mode-X colour fill, tab.md §6 / ke_dump render_level) as the clear colour;
        // every tile then draws over it with its top-left pivot at the given target pixel.
        target->compose([&] {
            neutrino::sprite_batch batch; // screen-space: add() positions are texture pixels
            const auto put = [&](const neutrino::sprite_set_handle& set, std::size_t i,
                                 int x, int y, float depth) {
                batch.add(neutrino::world_point{static_cast<float>(x), static_cast<float>(y)},
                          depth, set.require_visual(i));
            };

            // Score dummies, centred across the top row (depth 0).
            const neutrino::rect d0 = fill.require_frame_rect(ke_fill_score_left);
            const neutrino::rect d1 = fill.require_frame_rect(ke_fill_score_mid);
            const neutrino::rect d2 = fill.require_frame_rect(ke_fill_score_right);
            const int gap = (ke_screen_w - (d0.w + d1.w + d2.w)) / 2;
            put(fill, ke_fill_score_left, 0, 0, 0.0f);
            put(fill, ke_fill_score_mid, d0.w + gap, 0, 0.0f);
            put(fill, ke_fill_score_right, d0.w + gap + d1.w + gap, 0, 0.0f);
            const int h = d0.h;

            // Tiled KE_FILL block from y=16 across the screen (depth 1, under the walls). The
            // frame size is the loop STEP, so a degenerate 0x0 frame would spin forever -- assert
            // it rather than relying on the old `if (w > 0 && h > 0)` guard, which quietly skipped
            // the whole fill (a black playfield) when the block was missing.
            const neutrino::rect fr = fill.require_frame_rect(fill_block);
            ENFORCE(fr.w > 0 && fr.h > 0)("ke: KE_FILL block ", fill_block, " has degenerate size");
            for (int y = 16; y < ke_screen_h; y += fr.h) {
                for (int x = 0; x < ke_screen_w; x += fr.w) {
                    put(fill, fill_block, x, y, 1.0f);
                }
            }

            // KE_BORD wall pillars at y=h (depth 2, over the fill).
            const neutrino::rect rb = board.require_frame_rect(ke_bord_right);
            put(board, ke_bord_left, 0, h, 2.0f);
            put(board, ke_bord_right, ke_screen_w - rb.w, h, 2.0f);

            // KE_FILL horizontal bar under the score row (depth 3).
            put(fill, ke_fill_bar, 16, h, 3.0f);

            batch.flush();
        }, sdlpp::color{6, 34, 42, 255});

        return target;
    }
}
