#include "lens_effect.hh"

#include <cmath>
#include <algorithm>
#include <cstring>

namespace demoscene {

lens_effect::lens_effect()
    : m_displacement_map(diameter * diameter * 2, 0)
    , m_background(vga_canvas::pixel_count, 0)
{
    on_enter();
}

void lens_effect::precompute_lens() {
    // Calculate hemispherical displacement vector field (Δx, Δy)
    constexpr float r_sq = static_cast<float>(radius * radius);

    for (int dy = -radius; dy < radius; ++dy) {
        for (int dx = -radius; dx < radius; ++dx) {
            const float dist_sq = static_cast<float>(dx * dx + dy * dy);
            const size_t idx = static_cast<size_t>((dy + radius) * diameter + (dx + radius)) * 2;

            if (dist_sq < r_sq) {
                const float z = std::sqrt(r_sq - dist_sq);
                // Displacement vector pointing outward/magnifying
                const float factor = (static_cast<float>(radius) - z) * 0.45f;
                const float dist = std::sqrt(dist_sq);

                if (dist > 1e-3f) {
                    const int8_t disp_x = static_cast<int8_t>((static_cast<float>(dx) / dist) * factor);
                    const int8_t disp_y = static_cast<int8_t>((static_cast<float>(dy) / dist) * factor);
                    m_displacement_map[idx + 0] = disp_x;
                    m_displacement_map[idx + 1] = disp_y;
                }
            } else {
                m_displacement_map[idx + 0] = 0;
                m_displacement_map[idx + 1] = 0;
            }
        }
    }
}

void lens_effect::build_background() {
    // Generate a vibrant retro checkerboard backdrop with embedded geometric rings
    for (int y = 0; y < vga_canvas::height; ++y) {
        for (int x = 0; x < vga_canvas::width; ++x) {
            const int check = ((x / 20) + (y / 20)) % 2;
            const float dx = static_cast<float>(x - 160);
            const float dy = static_cast<float>(y - 100);
            const float dist = std::sqrt(dx * dx + dy * dy);

            uint8_t color_idx = check ? 20 : 10;

            // Concentric target rings across backdrop
            if (static_cast<int>(dist) % 30 < 4) {
                color_idx = 45; // Golden rings
            }

            m_background[static_cast<size_t>(y * vga_canvas::width + x)] = color_idx;
        }
    }
}

void lens_effect::on_enter() {
    precompute_lens();
    build_background();

    // Setup palette:
    // 0 = black
    // 10 = deep navy / slate blue
    // 20 = warm rust orange
    // 45 = bright gold
    // 50..60 = lens metallic rim
    m_canvas.set_rgb(0, 0, 0, 0);
    m_canvas.set_rgb(10, 25, 35, 65);
    m_canvas.set_rgb(20, 180, 70, 40);
    m_canvas.set_rgb(45, 255, 200, 50);

    // Lens golden brass rim
    for (int i = 50; i <= 60; ++i) {
        const float t = static_cast<float>(i - 50) / 10.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(180 + t * 75),
            static_cast<uint8_t>(140 + t * 90),
            static_cast<uint8_t>(40 + t * 100));
    }
}

void lens_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());

    // Interactive controls
    if (in.pressed(sdlpp::scancode::space)) {
        m_auto_bounce = !m_auto_bounce;
    }

    if (in.held(sdlpp::scancode::left))  { m_lens_x -= 120.0f * dt_sec; m_auto_bounce = false; }
    if (in.held(sdlpp::scancode::right)) { m_lens_x += 120.0f * dt_sec; m_auto_bounce = false; }
    if (in.held(sdlpp::scancode::up))    { m_lens_y -= 120.0f * dt_sec; m_auto_bounce = false; }
    if (in.held(sdlpp::scancode::down))  { m_lens_y += 120.0f * dt_sec; m_auto_bounce = false; }

    if (in.pressed(sdlpp::scancode::r)) {
        m_lens_x = 160.0f;
        m_lens_y = 100.0f;
        m_auto_bounce = true;
    }

    if (m_auto_bounce) {
        m_bounce_phase_x += euler::radian<float>(1.5f * dt_sec);
        m_bounce_phase_y += euler::radian<float>(2.1f * dt_sec);

        m_lens_x = 160.0f + 110.0f * std::cos(m_bounce_phase_x.value());
        m_lens_y = 100.0f + 55.0f * std::sin(m_bounce_phase_y.value());
    }

    m_lens_x = std::clamp(m_lens_x, static_cast<float>(radius), static_cast<float>(vga_canvas::width - radius));
    m_lens_y = std::clamp(m_lens_y, static_cast<float>(radius), static_cast<float>(vga_canvas::height - radius));
}

void lens_effect::render(const neutrino::rect& viewport) {
    // 1. Copy unmodified background to canvas
    std::memcpy(m_canvas.raw_pixels(), m_background.data(), vga_canvas::pixel_count);

    const int origin_x = static_cast<int>(m_lens_x) - radius;
    const int origin_y = static_cast<int>(m_lens_y) - radius;

    // 2. Refract pixels within the lens bounds
    constexpr int r_sq = radius * radius;

    for (int dy = 0; dy < diameter; ++dy) {
        const int py = origin_y + dy;
        if (py < 0 || py >= vga_canvas::height) continue;

        for (int dx = 0; dx < diameter; ++dx) {
            const int px = origin_x + dx;
            if (px < 0 || px >= vga_canvas::width) continue;

            const int rel_x = dx - radius;
            const int rel_y = dy - radius;
            const int dist_sq = rel_x * rel_x + rel_y * rel_y;

            if (dist_sq < r_sq) {
                const size_t disp_idx = static_cast<size_t>(dy * diameter + dx) * 2;
                const int sample_x = std::clamp(px - m_displacement_map[disp_idx + 0], 0, vga_canvas::width - 1);
                const int sample_y = std::clamp(py - m_displacement_map[disp_idx + 1], 0, vga_canvas::height - 1);

                m_canvas.put_pixel_fast(px, py, m_background[static_cast<size_t>(sample_y * vga_canvas::width + sample_x)]);
            } else if (dist_sq <= r_sq + (radius * 2)) {
                // Glass bezel rim
                m_canvas.put_pixel_fast(px, py, 55);
            }
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
