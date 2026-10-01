#include "dot_tunnel_effect.hh"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>

namespace demoscene {

dot_tunnel_effect::dot_tunnel_effect() {
    init_palette();
    init_ring_coords();
    on_enter();
}

void dot_tunnel_effect::init_ring_coords() {
    constexpr float pi2 = static_cast<float>(std::numbers::pi) * 2.0f;
    for (int j = 0; j < dots_per_ring; ++j) {
        const float theta = (static_cast<float>(j) / static_cast<float>(dots_per_ring)) * pi2;
        m_cos_table[static_cast<size_t>(j)] = std::cos(theta);
        m_sin_table[static_cast<size_t>(j)] = std::sin(theta);
    }
}

void dot_tunnel_effect::init_palette() {
    m_canvas.set_rgb(0, 0, 0, 0); // Black

    switch (m_theme) {
    case 0: { // Cyber Neon (Navy -> Cyan -> Magenta -> White)
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.5f) {
                const float u = t / 0.5f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(u * 40.0f),
                    static_cast<uint8_t>(40.0f + u * 180.0f),
                    static_cast<uint8_t>(120.0f + u * 135.0f));
            } else {
                const float u = (t - 0.5f) / 0.5f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(40.0f + u * 215.0f),
                    static_cast<uint8_t>(220.0f + u * 35.0f),
                    255);
            }
        }
        break;
    }
    case 1: { // Emerald Matrix (Deep forest to neon mint green)
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(t * t * 140.0f),
                static_cast<uint8_t>(30.0f + t * 225.0f),
                static_cast<uint8_t>(t * 120.0f));
        }
        break;
    }
    case 2: { // Solar Amber / Magma Core
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(std::min(255.0f, t * 300.0f)),
                static_cast<uint8_t>(t * t * 220.0f),
                static_cast<uint8_t>(t * t * t * 100.0f));
        }
        break;
    }
    case 3: { // Deep Space Ice (Pale Blue to Diamond White)
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(80.0f + t * 175.0f),
                static_cast<uint8_t>(120.0f + t * 135.0f),
                static_cast<uint8_t>(std::min(255.0f, 160.0f + t * 100.0f)));
        }
        break;
    }
    }
}

void dot_tunnel_effect::on_enter() {
    m_canvas.clear(0);
    init_palette();
    m_flight_z = 0.0f;
    m_flight_speed = 28.0f;
    m_sway_amp = 1.0f;
    m_time = 0.0f;
}

void dot_tunnel_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    if (in.pressed(sdlpp::scancode::space)) {
        m_theme = (m_theme + 1) % 4;
        init_palette();
    }

    if (in.pressed(sdlpp::scancode::t)) {
        m_spiral_twist = !m_spiral_twist;
    }

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }

    if (in.held(sdlpp::scancode::equals) || in.held(sdlpp::scancode::kp_plus)) {
        m_flight_speed = std::min(80.0f, m_flight_speed + 25.0f * dt_sec);
    }
    if (in.held(sdlpp::scancode::minus) || in.held(sdlpp::scancode::kp_minus)) {
        m_flight_speed = std::max(5.0f, m_flight_speed - 25.0f * dt_sec);
    }

    if (in.held(sdlpp::scancode::up))   m_sway_amp = std::min(2.5f, m_sway_amp + 1.0f * dt_sec);
    if (in.held(sdlpp::scancode::down)) m_sway_amp = std::max(0.0f, m_sway_amp - 1.0f * dt_sec);

    m_flight_z += m_flight_speed * dt_sec;
}

void dot_tunnel_effect::render(const neutrino::rect& viewport) {
    m_canvas.clear(0);
    uint8_t* raw = m_canvas.raw_pixels();

    constexpr float ring_spacing = 6.0f;
    constexpr float total_depth = static_cast<float>(num_rings) * ring_spacing;
    constexpr float tunnel_radius = 85.0f;
    constexpr float fov_scale = 165.0f;

    // Render rings from back to front (Painter's algorithm)
    for (int ring_idx = num_rings - 1; ring_idx >= 0; --ring_idx) {
        const float base_z = static_cast<float>(ring_idx) * ring_spacing;
        float z = std::fmod(base_z - m_flight_z, total_depth);
        if (z < 0.0f) z += total_depth;

        if (z < 3.5f) continue; // Skip rings behind or clipping camera

        // Compound Lissajous camera sway along tunnel depth
        const float sway_x = std::sin(z * 0.022f + m_time * 2.2f) * 48.0f * m_sway_amp;
        const float sway_y = std::cos(z * 0.018f - m_time * 1.8f) * 36.0f * m_sway_amp;

        // Spiral rotation angle per ring
        const float twist = m_spiral_twist ? (z * 0.035f + m_time * 1.5f) : 0.0f;
        const float cos_twist = std::cos(twist);
        const float sin_twist = std::sin(twist);

        const float inv_z = fov_scale / z;
        const float depth_ratio = std::clamp((total_depth - z) / total_depth, 0.0f, 1.0f);
        const uint8_t dot_color = static_cast<uint8_t>(20.0f + depth_ratio * 235.0f);

        for (int j = 0; j < dots_per_ring; ++j) {
            const float unrot_x = m_cos_table[static_cast<size_t>(j)];
            const float unrot_y = m_sin_table[static_cast<size_t>(j)];

            const float rot_x = unrot_x * cos_twist - unrot_y * sin_twist;
            const float rot_y = unrot_x * sin_twist + unrot_y * cos_twist;

            const float world_x = rot_x * tunnel_radius + sway_x;
            const float world_y = rot_y * tunnel_radius + sway_y;

            const int sx = 160 + static_cast<int>(world_x * inv_z);
            const int sy = 100 + static_cast<int>(world_y * inv_z);

            if (sx >= 0 && sx < vga_canvas::width && sy >= 0 && sy < vga_canvas::height) {
                raw[sy * vga_canvas::width + sx] = dot_color;

                // Close particles get drawn larger (2x2 with glow)
                if (z < 32.0f) {
                    if (sx + 1 < vga_canvas::width) raw[sy * vga_canvas::width + sx + 1] = dot_color;
                    if (sy + 1 < vga_canvas::height) raw[(sy + 1) * vga_canvas::width + sx] = dot_color;
                    if (sx + 1 < vga_canvas::width && sy + 1 < vga_canvas::height) {
                        raw[(sy + 1) * vga_canvas::width + sx + 1] = dot_color;
                    }
                }
            }
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
