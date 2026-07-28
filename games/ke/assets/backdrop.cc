//
// See backdrop.hh.
//

#include <ke/assets/backdrop.hh>

#include <neutrino/video/world/sprite_batch.hh>
#include <neutrino/world/world_common.hh>   // world_point
#include <ke/assets/sprites.hh>             // ke_paddle_frame (active form)

namespace rs {
    playfield_geometry compute_playfield_geometry(const neutrino::sprite_set_handle& board,
                                                  const neutrino::sprite_set_handle& fill,
                                                  const neutrino::sprite_set_handle& paddle) {
        // The score row sits at the top; the pillars and bar start just below it.
        const int score_h = fill.frame_rect(ke_fill_score_left).value_or(neutrino::rect{}).h;
        const neutrino::rect l = board.frame_rect(ke_bord_left).value_or(neutrino::rect{});
        const neutrino::rect r = board.frame_rect(ke_bord_right).value_or(neutrino::rect{});
        const neutrino::rect bar = fill.frame_rect(ke_fill_bar).value_or(neutrino::rect{});

        playfield_geometry geo{};
        geo.left_margin = l.w;
        geo.right_margin = ke_screen_w - r.w;
        geo.top_margin = score_h + bar.h;
        geo.bottom_margin = ke_screen_h;

        // Centre + bottom-align the paddle to its ACTIVE (default) form. game_mechanics builds
        // the collider from this exact frame and launches the ball off it, so using the
        // bounding cell of all forms would start the paddle off-centre and above the paddle row.
        const neutrino::rect form = paddle.frame_rect(
            rs::ke_paddle_frame(rs::ke_paddle_state::simple, rs::ke_paddle_default_size)).value_or(neutrino::rect{});
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
                          depth, set.visual(i));
            };

            // Score dummies, centred across the top row (depth 0).
            const neutrino::rect d0 = fill.frame_rect(ke_fill_score_left).value_or(neutrino::rect{});
            const neutrino::rect d1 = fill.frame_rect(ke_fill_score_mid).value_or(neutrino::rect{});
            const neutrino::rect d2 = fill.frame_rect(ke_fill_score_right).value_or(neutrino::rect{});
            const int gap = (ke_screen_w - (d0.w + d1.w + d2.w)) / 2;
            put(fill, ke_fill_score_left, 0, 0, 0.0f);
            put(fill, ke_fill_score_mid, d0.w + gap, 0, 0.0f);
            put(fill, ke_fill_score_right, d0.w + gap + d1.w + gap, 0, 0.0f);
            const int h = d0.h;

            // Tiled KE_FILL block from y=16 across the screen (depth 1, under the walls).
            if (const neutrino::rect fr = fill.frame_rect(fill_block).value_or(neutrino::rect{}); fr.w > 0 && fr.h > 0) {
                for (int y = 16; y < ke_screen_h; y += fr.h) {
                    for (int x = 0; x < ke_screen_w; x += fr.w) {
                        put(fill, fill_block, x, y, 1.0f);
                    }
                }
            }

            // KE_BORD wall pillars at y=h (depth 2, over the fill).
            const neutrino::rect rb = board.frame_rect(ke_bord_right).value_or(neutrino::rect{});
            put(board, ke_bord_left, 0, h, 2.0f);
            put(board, ke_bord_right, ke_screen_w - rb.w, h, 2.0f);

            // KE_FILL horizontal bar under the score row (depth 3).
            put(fill, ke_fill_bar, 16, h, 3.0f);

            batch.flush();
        }, sdlpp::color{6, 34, 42, 255});

        return target;
    }
}
