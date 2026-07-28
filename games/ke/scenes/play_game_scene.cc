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
            const neutrino::rect fr = assets.balls.frame_rect(frame).value_or(neutrino::rect{});
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
            const rs::ke_anim& a = rs::hit_anim(fx.kind);
            const float frame_secs = a.ticks * rs::ke_tick_seconds;
            const std::size_t idx = std::min <std::size_t>(a.count - 1,
                                                           frame_secs > 0.0f
                                                               ? static_cast <std::size_t>(fx.elapsed / frame_secs)
                                                               : 0);
            const std::size_t block = a.frames[idx]; // block == frame index in the set

            const neutrino::rect fr = assets.balls.frame_rect(block).value_or(neutrino::rect{});
            const neutrino::world_point pos{
                fx.pos.x - static_cast <float>(fr.w) * 0.5f,
                fx.pos.y - static_cast <float>(fr.h) * 0.5f
            };
            batch.add(pos, fx.pos.y + 2000.0f, assets.balls.visual(block)); // +2000: above balls
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
    m_backdrop.reset(); // destroy the texture while the renderer is still live
    rs::clear_ke_assets();
}

void play_game_scene::update_physics(neutrino::frame_duration dt) {
    if (!m_ready) {
        return;
    }
    model::instance().set_paddle_target(m_paddle_target_x); // input -> domain intent
    m_mechanics.tick(model::instance(), dt.count() / 1000.0f); // physics resolves it vs the walls
}

void play_game_scene::render(neutrino::frame_duration) {
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
    batch.flush();
}

void play_game_scene::handle_action(const sdlpp::event& ev) {
    if (const auto* m = ev.as <sdlpp::mouse_motion_event>()) {
        m_paddle_target_x = neutrino::to_render_coords({m->x, m->y}).x;
    }
    if (const auto* m = ev.as <sdlpp::mouse_button_event>()) {
        if (m->down && m->get_button() == sdlpp::mouse_button::left) {
            LOG_ERROR("Down");
        }
    }
}

bool play_game_scene::is_opaque() const {
    return true;
}
