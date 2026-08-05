//
// Created by igor on 12/07/2026.
//

#include <cstddef>
#include <optional>

#include <failsafe/logger.hh>

#include <sdlpp/events/events.hh>

#include <neutrino/video/globals.hh>
#include <neutrino/video/world/sprite_batch.hh>

#include <ke/assets/sprites.hh>
#include <ke/assets/backdrop.hh>
#include <ke/scenes/play_game_scene.hh>
#include <ke/assets/registry.hh>
#include <ke/game/model.hh>
#include <ke/game/sfx.hh>

namespace {
    // The stacking order of the game's sprite categories, as bands rather than as constants
    // added to y. Within a band the sort key is still pos.y, so things overlap by position the
    // way they should; between bands the order is fixed no matter where anything is.
    //
    // These replace `y + 1000` / `y + 1500` / `y + 2000`, which worked only because the
    // playfield is 200px tall -- the offsets had to be bigger than any y the game could produce,
    // an assumption nothing checked and nothing stated.
    constexpr neutrino::draw_layer layer_playfield{0}; // bricks and the paddle, interleaved by y
    constexpr neutrino::draw_layer layer_balls{1};
    constexpr neutrino::draw_layer layer_capsules{2};
    constexpr neutrino::draw_layer layer_effects{3};   // impact sparks, over everything

    // Fill the batch with the game's sprites -- bricks, paddle, balls -- read from the model.
    //
    // This is the one place gameplay positions leave the strong vocabulary: the batch draws in
    // untyped world points, so every model position crosses through to_world_point() here and
    // nowhere else. The depth key stays a bare float -- it is a sort order, not a coordinate.
    void draw_bricks(neutrino::sprite_batch& batch) {
        const auto& assets = rs::require_ke_assets();
        if (!assets.bricks.valid()) {
            return;
        }
        // Alive and flung bricks both draw at their current pos (flung ones slide off-screen).
        for (const brick& b : model::instance().get_level_info().bricks) {
            batch.add(neutrino::to_world_point(b.pos), layer_playfield, b.pos.y,
                      assets.bricks.visual(static_cast <std::size_t>(b.frame)));
        }
    }

    void draw_paddle(neutrino::sprite_batch& batch) {
        const auto& assets = rs::require_ke_assets();
        if (!assets.paddle.valid()) {
            return;
        }
        const paddle_info& p = model::instance().get_paddle();
        const neutrino::world_point pos{static_cast <float>(p.x), static_cast <float>(p.y)};
        batch.add(pos, layer_playfield, pos.y, assets.paddle.visual(rs::ke_paddle_frame(p.state, p.size)));
    }

    void draw_balls(neutrino::sprite_batch& batch) {
        const auto& assets = rs::require_ke_assets();
        if (!assets.balls.valid()) {
            return;
        }
        for (const ball_state& ball : model::instance().get_level_info().balls) {
            if (!ball.active) {
                continue;
            }
            // ball.pos is the collider CENTRE; the sprite pivots top-left, so shift by half the
            // frame to centre the graphic on the body.
            const std::size_t frame = rs::ke_ball_frame(ball.kind, ball.size);
            const neutrino::rect fr = assets.balls.require_frame_rect(frame);
            const neutrino::world_delta to_top_left{
                -static_cast <float>(fr.w) * 0.5f, -static_cast <float>(fr.h) * 0.5f
            };
            batch.add(neutrino::to_world_point(ball.pos + to_top_left), layer_balls, ball.pos.y,
                      assets.balls.visual(frame));
        }
    }

    void draw_effects(neutrino::sprite_batch& batch) {
        const auto& assets = rs::require_ke_assets();
        if (!assets.balls.valid()) { return; } // both anims live in KE_SPELL
        for (const hit_effect& fx : model::instance().get_level_info().effects) {
            batch.add(neutrino::to_world_point(fx.pos), layer_effects, fx.pos.y, fx.state);
        }
    }

    void draw_capsules(neutrino::sprite_batch& batch) {
        const auto& assets = rs::require_ke_assets();
        if (!assets.balls.valid()) { return; }
        for (const capsule& c : model::instance().get_level_info().capsules) {
            batch.add(neutrino::to_world_point(c.pos), layer_capsules, c.pos.y, c.state);
        }
    }
}

void play_game_scene::on_enter() {
    model::instance().load_level();
    m_mechanics.load(model::instance()); // builds the physical world from the domain
    ke::audio::instance().load(*m_assets.m_resources); // load KE_MAIN.DIG SFX

    // Compose the level backdrop into an offscreen texture (blitted each frame in render()).
    m_backdrop = rs::compose_backdrop(m_assets.board, m_assets.fill,
                                      rs::ke_fill_block_for_level(model::instance().get_level()));
    if (!m_backdrop) {
        LOG_ERROR("ke: could not create the backdrop render target");
    }

    m_ready = true;
}

void play_game_scene::on_exit() {
    // Only what this scene owns. The assets stay published: they belong to the application and
    // outlive every scene, so re-entering gameplay does not have to re-publish them.
    model::instance().get_level_info().clear(); // unregisters the effect states first
    m_backdrop.reset(); // destroy the texture while the renderer is still live
}

void play_game_scene::fixed_update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    if (!m_ready) {
        return;
    }
    // Poll the frame's render-space pointer (no event gating, no manual coord conversion) and hand
    // it to the domain as the paddle-centre target; physics resolves it against the walls. dt is a
    // constant seconds tick -- no ms/1000 conversion any more.
    //
    // Only when the pointer is actually over the window: off-screen the reported position is not
    // meaningful, and steering from it would drag the paddle to the left wall before the player has
    // touched the mouse. The paddle simply holds its last target instead.
    if (in.pointer().on_screen) {
        model::instance().set_paddle_target(static_cast <int>(in.pointer().render.x));
    }
    m_mechanics.tick(model::instance(), dt);
}

void play_game_scene::render() {
    if (!m_ready || !m_backdrop) {
        return;
    }
    const auto v = neutrino::render_size();
    const neutrino::rect viewport{0, 0, v.width, v.height};

    // 1. The static backdrop texture, scaled to the target.
    m_backdrop->blit(viewport);

    // 2. The game's sprites (bricks, paddle, balls). KE runs at a logical 320x200, so world
    //    positions are already screen pixels -- a plain screen-space batch, no camera.
    neutrino::sprite_batch batch;
    draw_bricks(batch);
    draw_paddle(batch);
    draw_balls(batch);
    draw_effects(batch);
    draw_capsules(batch);
    batch.flush();
}

void play_game_scene::handle_action(const sdlpp::event&) {
    // Continuous input (the paddle-tracking mouse position) is polled from the frame's
    // input_snapshot in fixed_update; only discrete one-shot events would be handled here.
}

bool play_game_scene::is_opaque() const {
    return true;
}
