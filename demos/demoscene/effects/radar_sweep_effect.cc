#include "radar_sweep_effect.hh"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace demoscene {

radar_sweep_effect::radar_sweep_effect() {
    on_enter();
}

void radar_sweep_effect::init_palette() {
    // 0: CRT tube black/dark glass
    m_canvas.set_rgb(0, 4, 6, 8);

    // 1..30: Dim scope reticle / grid
    switch (m_color_theme) {
    case 0: { // P31 Green Phosphor
        for (int i = 1; i <= 30; ++i) {
            const float t = static_cast<float>(i) / 30.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(t * 10),
                static_cast<uint8_t>(20 + t * 45),
                static_cast<uint8_t>(t * 15));
        }
        // 31..230: Phosphor decay trail
        for (int i = 31; i <= 230; ++i) {
            const float t = static_cast<float>(i - 31) / 199.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(t * 60),
                static_cast<uint8_t>(60 + t * 180),
                static_cast<uint8_t>(t * 80));
        }
        // 231..255: Core beam & target blip saturation (bright white-green)
        for (int i = 231; i <= 255; ++i) {
            const float t = static_cast<float>(i - 231) / 24.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(80 + t * 175),
                255,
                static_cast<uint8_t>(100 + t * 155));
        }
        break;
    }
    case 1: { // P20 Amber Radar
        for (int i = 1; i <= 30; ++i) {
            const float t = static_cast<float>(i) / 30.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(25 + t * 45),
                static_cast<uint8_t>(10 + t * 25),
                0);
        }
        for (int i = 31; i <= 230; ++i) {
            const float t = static_cast<float>(i - 31) / 199.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(80 + t * 165),
                static_cast<uint8_t>(40 + t * 150),
                static_cast<uint8_t>(t * 30));
        }
        for (int i = 231; i <= 255; ++i) {
            const float t = static_cast<float>(i - 231) / 24.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                255,
                static_cast<uint8_t>(190 + t * 65),
                static_cast<uint8_t>(60 + t * 195));
        }
        break;
    }
    case 2: { // Sonar Marine Blue
        for (int i = 1; i <= 30; ++i) {
            const float t = static_cast<float>(i) / 30.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                0,
                static_cast<uint8_t>(15 + t * 35),
                static_cast<uint8_t>(25 + t * 45));
        }
        for (int i = 31; i <= 230; ++i) {
            const float t = static_cast<float>(i - 31) / 199.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(t * 40),
                static_cast<uint8_t>(60 + t * 150),
                static_cast<uint8_t>(90 + t * 160));
        }
        for (int i = 231; i <= 255; ++i) {
            const float t = static_cast<float>(i - 231) / 24.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(100 + t * 155),
                static_cast<uint8_t>(210 + t * 45),
                255);
        }
        break;
    }
    default:
        break;
    }
}

void radar_sweep_effect::reset_targets() {
    m_targets.clear();
    // 7 initial radar contacts with realistic drifting trajectories
    m_targets.push_back({{130.0f,  60.0f}, { 4.0f,  2.0f}, 0.0f});
    m_targets.push_back({{205.0f,  75.0f}, {-3.0f,  1.5f}, 0.0f});
    m_targets.push_back({{180.0f, 140.0f}, {-2.5f, -3.0f}, 0.0f});
    m_targets.push_back({{110.0f, 125.0f}, { 3.5f, -1.0f}, 0.0f});
    m_targets.push_back({{160.0f,  45.0f}, { 1.5f,  3.0f}, 0.0f});
    m_targets.push_back({{225.0f, 120.0f}, {-4.0f, -2.0f}, 0.0f});
    m_targets.push_back({{140.0f, 160.0f}, { 2.0f, -2.5f}, 0.0f});
}

void radar_sweep_effect::on_enter() {
    init_palette();
    m_canvas.clear(0);
    reset_targets();
    m_angle = 0.0f;
    m_sweep_speed = 2.0f;
    m_time = 0.0f;
    m_show_grid = true;
    m_color_theme = 0;
}

void radar_sweep_effect::draw_line(int x0, int y0, int x1, int y1, uint8_t color) {
    // Bresenham line algorithm (matching Bas van Gaalen's SWEEP.PAS line routine)
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        m_canvas.put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void radar_sweep_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    // Advance sweeping radar beam angle
    const float prev_angle = m_angle;
    m_angle += m_sweep_speed * dt_sec;
    constexpr float two_pi = 2.0f * std::numbers::pi_v<float>;
    if (m_angle >= two_pi) m_angle -= two_pi;
    if (m_angle < 0.0f) m_angle += two_pi;

    // Controls
    if (in.pressed(sdlpp::scancode::space)) {
        reset_targets();
    }

    if (in.pressed(sdlpp::scancode::c)) {
        m_color_theme = (m_color_theme + 1) % 3;
        init_palette();
    }

    if (in.pressed(sdlpp::scancode::m)) {
        m_show_grid = !m_show_grid;
    }

    if (in.held(sdlpp::scancode::up))   m_sweep_speed = std::min(6.0f, m_sweep_speed + 2.0f * dt_sec);
    if (in.held(sdlpp::scancode::down)) m_sweep_speed = std::max(0.5f, m_sweep_speed - 2.0f * dt_sec);

    if (in.pressed(sdlpp::scancode::r)) {
        m_angle = 0.0f;
        m_sweep_speed = 2.0f;
        m_show_grid = true;
        m_color_theme = 0;
        reset_targets();
        init_palette();
        m_canvas.clear(0);
    }

    // Step target positions and detect beam intersections
    constexpr float cx = 160.0f;
    constexpr float cy = 100.0f;
    constexpr float max_radius = 90.0f;

    for (auto& t : m_targets) {
        t.pos.x() += t.vel.x() * dt_sec;
        t.pos.y() += t.vel.y() * dt_sec;

        // Bounce back if drifting outside radar scope circle
        const float dx = t.pos.x() - cx;
        const float dy = t.pos.y() - cy;
        const float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > max_radius - 4.0f) {
            t.vel.x() = -t.vel.x();
            t.vel.y() = -t.vel.y();
            t.pos.x() = cx + (dx / dist) * (max_radius - 5.0f);
            t.pos.y() = cy + (dy / dist) * (max_radius - 5.0f);
        }

        // Target azimuth angle in [0, 2pi)
        float target_angle = std::atan2(dy, dx);
        if (target_angle < 0.0f) target_angle += two_pi;

        // Check if sweeping beam crossed target angle during this frame
        float diff = m_angle - target_angle;
        while (diff > std::numbers::pi_v<float>) diff -= two_pi;
        while (diff < -std::numbers::pi_v<float>) diff += two_pi;

        if (std::abs(diff) < 0.12f) {
            t.intensity = 1.0f; // Illuminate target with maximum brightness!
        } else {
            // Gradual phosphor decay of target contact
            t.intensity = std::max(0.0f, t.intensity - 0.45f * dt_sec);
        }
    }
}

void radar_sweep_effect::render(const neutrino::rect& viewport) {
    constexpr int cx = 160;
    constexpr int cy = 100;
    constexpr int scope_radius = 92;

    // 1. Phosphor Persistence Decay: fade previous frame pixels
    uint8_t* raw = m_canvas.raw_pixels();
    for (int i = 0; i < vga_canvas::pixel_count; ++i) {
        if (raw[i] > 30) {
            // Decay trail pixels down toward baseline
            raw[i] = (raw[i] > 33) ? (raw[i] - 3) : 0;
        } else if (raw[i] > 0 && !m_show_grid) {
            raw[i] = 0;
        }
    }

    // 2. Draw Scope Range Rings & Reticle
    if (m_show_grid) {
        // Draw concentric range rings (radii 30, 60, 90)
        const int ring_radii[] = {30, 60, scope_radius};
        for (int r : ring_radii) {
            constexpr int steps = 180;
            for (int s = 0; s < steps; ++s) {
                const float a = static_cast<float>(s) * (6.283185f / static_cast<float>(steps));
                const int px = cx + static_cast<int>(std::cos(a) * static_cast<float>(r));
                const int py = cy + static_cast<int>(std::sin(a) * static_cast<float>(r));
                // Only overwrite if background (do not overwrite bright phosphor trails)
                if (m_canvas.get_pixel(px, py) <= 30) {
                    m_canvas.put_pixel(px, py, 18);
                }
            }
        }

        // Draw crosshair axes
        for (int x = cx - scope_radius; x <= cx + scope_radius; x += 2) {
            if (m_canvas.get_pixel(x, cy) <= 30) m_canvas.put_pixel(x, cy, 14);
        }
        for (int y = cy - scope_radius; y <= cy + scope_radius; y += 2) {
            if (m_canvas.get_pixel(cx, y) <= 30) m_canvas.put_pixel(cx, y, 14);
        }
    }

    // 3. Render Target Blips with glowing halos
    for (const auto& t : m_targets) {
        if (t.intensity > 0.05f) {
            const int tx = static_cast<int>(t.pos.x());
            const int ty = static_cast<int>(t.pos.y());
            const uint8_t blip_val = static_cast<uint8_t>(200.0f + t.intensity * 55.0f);

            // Core dot
            m_canvas.put_pixel(tx, ty, blip_val);
            m_canvas.put_pixel(tx + 1, ty, blip_val);
            m_canvas.put_pixel(tx - 1, ty, blip_val);
            m_canvas.put_pixel(tx, ty + 1, blip_val);
            m_canvas.put_pixel(tx, ty - 1, blip_val);

            // Heading vector velocity line
            const int hx = tx + static_cast<int>(t.vel.x() * 1.5f);
            const int hy = ty + static_cast<int>(t.vel.y() * 1.5f);
            draw_line(tx, ty, hx, hy, static_cast<uint8_t>(160.0f * t.intensity));
        }
    }

    // 4. Render Sweeping Radar Beam (SWEEP.PAS polar line sweep)
    const int beam_end_x = cx + static_cast<int>(std::cos(m_angle) * static_cast<float>(scope_radius));
    const int beam_end_y = cy + static_cast<int>(std::sin(m_angle) * static_cast<float>(scope_radius));

    // Draw main bright beam
    draw_line(cx, cy, beam_end_x, beam_end_y, 255);

    // Draw trailing soft edge rays for realistic beam thickness
    const float trail_angle1 = m_angle - 0.035f;
    const int trail_end_x1 = cx + static_cast<int>(std::cos(trail_angle1) * static_cast<float>(scope_radius));
    const int trail_end_y1 = cy + static_cast<int>(std::sin(trail_angle1) * static_cast<float>(scope_radius));
    draw_line(cx, cy, trail_end_x1, trail_end_y1, 210);

    const float trail_angle2 = m_angle - 0.070f;
    const int trail_end_x2 = cx + static_cast<int>(std::cos(trail_angle2) * static_cast<float>(scope_radius));
    const int trail_end_y2 = cy + static_cast<int>(std::sin(trail_angle2) * static_cast<float>(scope_radius));
    draw_line(cx, cy, trail_end_x2, trail_end_y2, 160);

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
