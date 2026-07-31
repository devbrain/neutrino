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
    // Fill the batch with the game's sprites -- bricks, paddle, balls -- read from the model.
    void draw_bricks(neutrino::sprite_batch& batch) {
        const auto& assets = rs::require_ke_assets();
        if (!assets.bricks.valid()) {
            return;
        }
        // Alive and flung bricks both draw at their current pos (flung ones slide off-screen).
        for (const brick& b : model::instance().get_level_info().bricks) {
            batch.add(b.pos, b.pos.y, assets.bricks.visual(static_cast <std::size_t>(b.frame)));
        }
    }

    void draw_paddle(neutrino::sprite_batch& batch) {
        const auto& assets = rs::require_ke_assets();
        if (!assets.paddle.valid()) {
            return;
        }
        const paddle_info& p = model::instance().get_paddle();
        const neutrino::world_point pos{static_cast <float>(p.x), static_cast <float>(p.y)};
        batch.add(pos, pos.y, assets.paddle.visual(rs::ke_paddle_frame(p.state, p.size)));
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
            // frame to centre the graphic on the body. +1000 depth: above bricks/paddle.
            const std::size_t frame = rs::ke_ball_frame(ball.kind, ball.size);
            const neutrino::rect fr = assets.balls.require_frame_rect(frame);
            const neutrino::world_point pos{
                ball.pos.x - static_cast <float>(fr.w) * 0.5f,
                ball.pos.y - static_cast <float>(fr.h) * 0.5f
            };
            batch.add(pos, ball.pos.y + 1000.0f, assets.balls.visual(frame));
        }
    }

    void draw_effects(neutrino::sprite_batch& batch) {
        const auto& assets = rs::require_ke_assets();
        if (!assets.balls.valid()) { return; } // both anims live in KE_SPELL
        for (const hit_effect& fx : model::instance().get_level_info().effects) {
            batch.add(fx.pos, fx.pos.y + 2000.0f, fx.state);
        }
    }

    void draw_capsules(neutrino::sprite_batch& batch) {
        const auto& assets = rs::require_ke_assets();
        if (!assets.balls.valid()) { return; }
        for (const capsule& c : model::instance().get_level_info().capsules) {
            batch.add(c.pos, c.pos.y + 1500.0f, c.state); // above bricks/paddle, below sparks (2000)
        }
    }
}

void play_game_scene::on_enter() {
    auto& assets = rs::require_ke_assets();
    model::instance().load_level();
    m_mechanics.load(model::instance()); // builds the physical world from the domain
    ke::audio::instance().load(*assets.m_resources); // load KE_MAIN.DIG SFX

    // Compose the level backdrop into an offscreen texture (blitted each frame in render()).
    m_backdrop = rs::compose_backdrop(assets.board, assets.fill,
                                      rs::ke_fill_block_for_level(model::instance().get_level()));
    if (!m_backdrop) {
        LOG_ERROR("ke: could not create the backdrop render target");
    }

    m_ready = true;
}

void play_game_scene::on_exit() {
    model::instance().get_level_info().clear(); // unregisters the effect states first
    m_backdrop.reset(); // destroy the texture while the renderer is still live
    rs::clear_ke_assets();
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
    m_mechanics.tick(model::instance(), dt.count());
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
