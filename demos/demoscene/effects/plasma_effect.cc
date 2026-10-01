#include "plasma_effect.hh"

#include <cmath>
#include <algorithm>

namespace demoscene {

plasma_effect::plasma_effect() {
    on_enter();
}

void plasma_effect::rebuild_palette() {
    // Reconstruct the classic psychedelic 4-phase demoscene cycle palette:
    // Red -> Gold -> Cyan/Aquamarine -> Electric Violet -> Red
    m_canvas.set_rgb(0, 0, 0, 0);

    for (int i = 1; i <= 255; ++i) {
        const float t = static_cast<float>(i - 1) / 254.0f;
        const float angle = t * 6.2831853f; // 2π

        // Sinusoidal color components shifted by 120° (2π/3)
        const uint8_t r = static_cast<uint8_t>(127.0f + 127.0f * std::sin(angle));
        const uint8_t g = static_cast<uint8_t>(127.0f + 127.0f * std::sin(angle + 2.0943951f));
        const uint8_t b = static_cast<uint8_t>(127.0f + 127.0f * std::sin(angle + 4.1887902f));

        m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
    }
}

void plasma_effect::on_enter() {
    rebuild_palette();
    m_canvas.clear(0);
}

void plasma_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());

    // Interactive controls
    if (in.pressed(sdlpp::scancode::p)) m_cycling_enabled = !m_cycling_enabled;
    if (in.held(sdlpp::scancode::right)) m_cycle_speed = std::min(10, m_cycle_speed + 1);
    if (in.held(sdlpp::scancode::left))  m_cycle_speed = std::max(-10, m_cycle_speed - 1);

    if (in.pressed(sdlpp::scancode::r)) {
        m_cycle_speed = 2;
        m_cycling_enabled = true;
        rebuild_palette();
    }

    // Advance wave phases using euler::radian
    m_phase_x += euler::radian<float>(1.2f * dt_sec);
    m_phase_y += euler::radian<float>(1.6f * dt_sec);
    m_phase_rad += euler::radian<float>(2.0f * dt_sec);

    // 1990s Hardware Palette Cycling: rotate colors in DAC registers
    if (m_cycling_enabled && m_cycle_speed != 0) {
        m_canvas.cycle_palette(1, 255, m_cycle_speed);
    }
}

void plasma_effect::render(const neutrino::rect& viewport) {
    // 2D Trigonometric Superposition Field
    constexpr float center_x = 160.0f;
    constexpr float center_y = 100.0f;

    const float ph_x = m_phase_x.value();
    const float ph_y = m_phase_y.value();
    const float ph_rad = m_phase_rad.value();

    for (int y = 0; y < vga_canvas::height; ++y) {
        const float fy = static_cast<float>(y);
        const float dy = fy - center_y;
        const float dy_sq = dy * dy;

        const float wave_y = std::sin(fy * 0.05f + ph_y);

        for (int x = 0; x < vga_canvas::width; ++x) {
            const float fx = static_cast<float>(x);
            const float dx = fx - center_x;

            const float wave_x = std::sin(fx * 0.04f + ph_x);
            const float wave_diag = std::sin((fx + fy) * 0.03f + ph_x * 0.5f);
            const float dist = std::sqrt(dx * dx + dy_sq);
            const float wave_radial = std::sin(dist * 0.07f - ph_rad);

            // Sum of 4 sinusoidal fields: range [-4.0, 4.0]
            const float sum = wave_x + wave_y + wave_diag + wave_radial;

            // Map [-4, 4] to palette indices [1, 255]
            const int color_idx = 1 + static_cast<int>((sum + 4.0f) * 31.75f);
            m_canvas.put_pixel_fast(x, y, static_cast<uint8_t>(std::clamp(color_idx, 1, 255)));
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
