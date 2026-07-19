//
// Created by igor on 12/07/2026.
//

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>

#include <failsafe/logger.hh>

#include <sdlpp/events/events.hh>

#include <neutrino/video/globals.hh>

#include <ke/assets/sprites.hh>
#include <ke/assets/world.hh>
#include <ke/scenes/play_game_scene.hh>
#include <ke/assets/registry.hh>
#include <ke/game/model.hh>

namespace {
    // Playfield wall x the paddle rides between (tab.md §6/§7).
    constexpr float wall_left = 16.0f;
    constexpr float wall_right = 304.0f;
    // Paddle bottom edge in the empty bottom band.
    constexpr float paddle_bottom_y = 190.0f;
    // The default paddle form: simple state, size 7.
    constexpr std::size_t paddle_frame = rs::ke_paddle_frame(rs::ke_paddle_state::simple, rs::ke_paddle_default_size);
}



void play_game_scene::on_enter() {

    auto& assets = rs::require_ke_assets();
    model::instance().load_level();

    // The background is per-level, parametrized by its fill_block (block 6 for now).

    m_renderer = std::make_unique <neutrino::world_renderer>(model::instance().get_visual_world());
    m_compositor = std::make_unique <neutrino::world_compositor>(*m_renderer);
    m_compositor->insert_after(rs::ke_bricks_layer_id, m_actors); // actors above bricks

    // Paddle: a visual from the leased paddle set (origins baked by the sprite pipeline).
    if (auto v = assets.paddle.visual(std::to_string(paddle_frame))) {
        //m_actors.paddle = *v;
        if (auto rack = assets.m_resources->tile_sheets.find("ke_rack");
            rack != assets.m_resources->tile_sheets.end() && paddle_frame < rack->second.source_rects.size()) {
            m_paddle_w = rack->second.source_rects[paddle_frame].w;
            m_paddle_h = rack->second.source_rects[paddle_frame].h;
        }
    } else {
        LOG_ERROR("ke: paddle visual missing");
    }

    m_ready = true;
}

void play_game_scene::on_exit() {
    // Release in dependency order while the render services are still live, then unpublish.
    m_compositor.reset();
    m_renderer.reset();
    rs::clear_ke_assets();
}

void play_game_scene::update_physics(neutrino::frame_duration) {
    if (!m_ready) {
        return;
    }
    const auto w = static_cast <float>(m_paddle_w);
    const float left = std::clamp(m_paddle_center_x - w * 0.5f, wall_left, wall_right - w);
 //   m_actors.paddle_pos = {left, paddle_bottom_y - static_cast <float>(m_paddle_h)};
}

void play_game_scene::render(neutrino::frame_duration) {
    if (!m_compositor) {
        return;
    }
    const auto v = neutrino::render_size();
    neutrino::camera cam;
    cam.target = {static_cast <float>(v.width) * 0.5f, static_cast <float>(v.height) * 0.5f};
    cam.zoom = 1.0f;

    const auto [drawn, skipped, failed] =
        m_compositor->draw(cam, neutrino::rect{0, 0, v.width, v.height});

    if (!m_logged_first_frame) {
        m_logged_first_frame = true;
        std::cerr << "ke: first frame drew " << drawn << " tiles ("
                  << skipped << " skipped, " << failed << " failed)\n";
    }
}

void play_game_scene::handle_action(const sdlpp::event& ev) {
    if (const auto* m = ev.as <sdlpp::mouse_motion_event>()) {
        m_paddle_center_x = neutrino::to_render_coords({m->x, m->y}).x;
    }
}

bool play_game_scene::is_opaque() const {
    return true;
}
