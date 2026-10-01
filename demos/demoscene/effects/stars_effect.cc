#include "stars_effect.hh"

#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <neutrino/video/draw.hh>

namespace demoscene {

stars_effect::stars_effect()
    : m_stars(star_count)
{
    on_enter();
}

void stars_effect::respawn_star(star& s, bool random_z) {
    const float rx = static_cast<float>(rand() % 800 - 400);
    const float ry = static_cast<float>(rand() % 600 - 300);
    const float rz = random_z ? static_cast<float>(rand() % 900 + 100) : 1000.0f;

    s.pos = euler::vec3<float>{rx, ry, rz};
    s.speed_scale = 0.8f + static_cast<float>(rand() % 40) * 0.01f;

    constexpr float focal_length = 250.0f;
    s.prev_proj_x = 320.0f + (rx * focal_length) / rz;
    s.prev_proj_y = 200.0f + (ry * focal_length) / rz;
}

void stars_effect::on_enter() {
    for (auto& s : m_stars) {
        respawn_star(s, true);
    }
}

void stars_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());

    // Interactive controls
    if (in.held(sdlpp::scancode::up))   m_warp_speed = std::min(1200.0f, m_warp_speed + 400.0f * dt_sec);
    if (in.held(sdlpp::scancode::down)) m_warp_speed = std::max(50.0f, m_warp_speed - 400.0f * dt_sec);

    m_hyperdrive = in.held(sdlpp::scancode::space);

    if (in.pressed(sdlpp::scancode::r)) {
        m_warp_speed = 350.0f;
        on_enter();
    }

    const float effective_speed = m_warp_speed * (m_hyperdrive ? 3.5f : 1.0f);

    // Advance stars toward camera (decreasing z)
    for (auto& s : m_stars) {
        s.pos.z() -= effective_speed * s.speed_scale * dt_sec;

        // If star passes behind the camera, respawn at far boundary
        if (s.pos.z() <= 10.0f) {
            respawn_star(s, false);
        }
    }
}

void stars_effect::render(const neutrino::rect& viewport) {
    // Fill deep interstellar black
    neutrino::draw_rect_fill(viewport, sdlpp::color{2, 4, 8, 255});

    const float center_x = static_cast<float>(viewport.x) + static_cast<float>(viewport.w) * 0.5f;
    const float center_y = static_cast<float>(viewport.y) + static_cast<float>(viewport.h) * 0.5f;
    constexpr float focal_length = 260.0f;

    for (auto& s : m_stars) {
        if (s.pos.z() <= 10.0f) continue;

        // Perspective divide: screen = center + (world * focal) / z
        const float inv_z = focal_length / s.pos.z();
        const float curr_x = center_x + s.pos.x() * inv_z;
        const float curr_y = center_y + s.pos.y() * inv_z;

        // Depth cueing: nearby stars are brighter and slightly larger
        const float z_factor = std::clamp(1.0f - s.pos.z() / 1000.0f, 0.0f, 1.0f);
        const uint8_t brightness = static_cast<uint8_t>(50 + z_factor * 205);
        const uint8_t alpha = static_cast<uint8_t>(80 + z_factor * 175);

        // Check if inside screen boundaries
        if (curr_x >= viewport.x && curr_x < viewport.x + viewport.w &&
            curr_y >= viewport.y && curr_y < viewport.y + viewport.h) {

            // Draw anti-aliased velocity streak from previous frame's projected point
            const neutrino::point p_prev{static_cast<int>(s.prev_proj_x), static_cast<int>(s.prev_proj_y)};
            const neutrino::point p_curr{static_cast<int>(curr_x), static_cast<int>(curr_y)};

            // Electric blue/white streak
            sdlpp::color streak_color{
                brightness,
                static_cast<uint8_t>(brightness * 0.95f),
                255,
                alpha
            };
            neutrino::draw_line_aa(p_prev, p_curr, streak_color);

            // Draw bright star head
            const int dot_radius = (z_factor > 0.7f) ? 2 : 1;
            neutrino::draw_circle_fill(
                static_cast<int>(curr_x), static_cast<int>(curr_y), dot_radius,
                sdlpp::color{255, 255, 255, alpha}
            );
        }

        s.prev_proj_x = curr_x;
        s.prev_proj_y = curr_y;
    }
}

} // namespace demoscene
