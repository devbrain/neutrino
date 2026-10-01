#include "bobs_effect.hh"

#include <cmath>
#include <algorithm>

namespace demoscene {

bobs_effect::bobs_effect() {
    on_enter();
}

void bobs_effect::on_enter() {
    m_canvas.clear(0);

    // Setup 64-color smooth translucent ramp (indices 0..63)
    // 0 = Void black
    m_canvas.set_rgb(0, 0, 0, 0);

    for (int i = 1; i <= 63; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        uint8_t r = 0, g = 0, b = 0;

        if (t < 0.33f) {
            // Indigo to Neon Purple
            const float k = t / 0.33f;
            r = static_cast<uint8_t>(40 + k * 140);
            g = static_cast<uint8_t>(k * 20);
            b = static_cast<uint8_t>(100 + k * 140);
        } else if (t < 0.66f) {
            // Purple to Burning Orange
            const float k = (t - 0.33f) / 0.33f;
            r = static_cast<uint8_t>(180 + k * 75);
            g = static_cast<uint8_t>(20 + k * 140);
            b = static_cast<uint8_t>(240 * (1.0f - k));
        } else {
            // Orange to Golden White
            const float k = (t - 0.66f) / 0.34f;
            r = 255;
            g = static_cast<uint8_t>(160 + k * 95);
            b = static_cast<uint8_t>(k * 230);
        }
        m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
    }
}

void bobs_effect::stamp_bob(int center_x, int center_y) {
    // 16x16 circular kernel with soft additive falloff
    constexpr int radius = 8;
    for (int dy = -radius; dy <= radius; ++dy) {
        const int py = center_y + dy;
        if (py < 0 || py >= vga_canvas::height) continue;

        for (int dx = -radius; dx <= radius; ++dx) {
            const int px = center_x + dx;
            if (px < 0 || px >= vga_canvas::width) continue;

            const int dist_sq = dx * dx + dy * dy;
            if (dist_sq <= radius * radius) {
                // Additive stamp value: 2 in center, 1 near boundary
                const uint8_t delta = (dist_sq < 16) ? 2 : 1;
                m_canvas.add_pixel(px, py, delta, 63);
            }
        }
    }
}

void bobs_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());

    // Controls
    if (in.pressed(sdlpp::scancode::c)) m_canvas.clear(0);
    if (in.pressed(sdlpp::scancode::d)) m_decay_enabled = !m_decay_enabled;

    if (in.held(sdlpp::scancode::up)) m_speed = std::min(3.0f, m_speed + 1.0f * dt_sec);
    if (in.held(sdlpp::scancode::down)) m_speed = std::max(0.2f, m_speed - 1.0f * dt_sec);

    if (in.pressed(sdlpp::scancode::r)) {
        m_canvas.clear(0);
        m_speed = 1.0f;
        m_decay_enabled = false;
    }

    // Optional decay simulation: gently diminish pixel values
    if (m_decay_enabled) {
        auto pixels = m_canvas.pixels();
        for (auto& px : pixels) {
            if (px > 0 && (rand() % 6 == 0)) {
                px--;
            }
        }
    }

    // Update 4 harmonic phases using euler::radian
    m_phase_a += euler::radian<float>(1.8f * m_speed * dt_sec);
    m_phase_b += euler::radian<float>(2.4f * m_speed * dt_sec);
    m_phase_c += euler::radian<float>(1.3f * m_speed * dt_sec);
    m_phase_d += euler::radian<float>(2.9f * m_speed * dt_sec);

    // Compute bob positions along multi-frequency Lissajous paths
    constexpr float mid_x = 160.0f;
    constexpr float mid_y = 100.0f;

    // Bob 1
    const int x1 = static_cast<int>(mid_x + 95.0f * std::sin(m_phase_a.value()) + 30.0f * std::cos(m_phase_c.value()));
    const int y1 = static_cast<int>(mid_y + 65.0f * std::cos(m_phase_b.value()) + 20.0f * std::sin(m_phase_d.value()));
    stamp_bob(x1, y1);

    // Bob 2 (counter-rotating harmonic)
    const int x2 = static_cast<int>(mid_x + 95.0f * std::cos(m_phase_b.value() * 0.8f) - 30.0f * std::sin(m_phase_a.value()));
    const int y2 = static_cast<int>(mid_y + 65.0f * std::sin(m_phase_a.value() * 1.2f) - 20.0f * std::cos(m_phase_c.value()));
    stamp_bob(x2, y2);
}

void bobs_effect::render(const neutrino::rect& viewport) {
    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
