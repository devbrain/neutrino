#include "wavy_ribbon_effect.hh"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace demoscene {

wavy_ribbon_effect::wavy_ribbon_effect() {
    init_tables();
    on_enter();
}

void wavy_ribbon_effect::init_tables() {
    constexpr float pi2 = static_cast<float>(std::numbers::pi * 2.0);
    for (int i = 0; i < 256; ++i) {
        const float t = static_cast<float>(i) / 255.0f;
        m_stab1[i] = static_cast<int>(std::round(std::sin(t * pi2) * 50.0f)) + 99;
        m_stab2[i] = static_cast<int>(std::round(std::cos(t * pi2 * 2.0f) * 25.0f));
        m_stab3[i] = static_cast<int>(std::round(std::sin(t * pi2 * 2.0f) * 25.0f));
    }
}

void wavy_ribbon_effect::on_enter() {
    m_canvas.clear(0);
    init_palette();
    m_ctab.fill(99);
    m_ptab.fill(99);
    m_i1 = 0;
    m_i2 = 25;
    m_i3 = 100;
    m_amplitude = 1.0f;
    m_speed = 1.0f;
}

void wavy_ribbon_effect::init_palette() {
    m_canvas.set_rgb(0, 4, 6, 12); // Midnight slate background

    switch (m_theme) {
    case 0: { // Authentic 1995 WAVY.PAS Seafoam / Pastel Copper
        for (int i = 1; i <= 255; ++i) {
            // port[$3C9] := i div 4; 20 + i div 5; 10 + i div 6;
            const uint8_t r6 = static_cast<uint8_t>(std::min(63, i / 4));
            const uint8_t g6 = static_cast<uint8_t>(std::min(63, 20 + i / 5));
            const uint8_t b6 = static_cast<uint8_t>(std::min(63, 10 + i / 6));
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(i), r6, g6, b6);
        }
        break;
    }
    case 1: { // Cyberpunk Neon (Hot Pink -> Laser Violet -> Electric Turquoise)
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.5f) {
                const float u = t / 0.5f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(255 - u * 60.0f),
                    static_cast<uint8_t>(20.0f + u * 20.0f),
                    static_cast<uint8_t>(120.0f + u * 135.0f));
            } else {
                const float u = (t - 0.5f) / 0.5f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(195.0f - u * 195.0f),
                    static_cast<uint8_t>(40.0f + u * 215.0f),
                    255);
            }
        }
        break;
    }
    case 2: { // Aurora Borealis (Ethereal Emerald & Indigo)
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(std::sin(t * 3.14159f) * 120.0f),
                static_cast<uint8_t>(40.0f + t * 215.0f),
                static_cast<uint8_t>(120.0f + (1.0f - t) * 135.0f));
        }
        break;
    }
    case 3: { // Golden Sunset (Deep Crimson -> Tangerine -> Radiant Gold)
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(std::min(255.0f, t * 300.0f)),
                static_cast<uint8_t>(t * t * 210.0f),
                static_cast<uint8_t>(t * t * t * 80.0f));
        }
        break;
    }
    }
}

void wavy_ribbon_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    if (in.pressed(sdlpp::scancode::space)) {
        m_mode = (m_mode + 1) % 4;
        m_canvas.clear(0);
    }

    if (in.pressed(sdlpp::scancode::c)) {
        m_theme = (m_theme + 1) % 4;
        init_palette();
    }

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }

    if (in.held(sdlpp::scancode::up))   m_amplitude = std::min(1.8f, m_amplitude + 0.8f * dt_sec);
    if (in.held(sdlpp::scancode::down)) m_amplitude = std::max(0.2f, m_amplitude - 0.8f * dt_sec);

    if (in.held(sdlpp::scancode::right)) m_speed = std::min(3.0f, m_speed + 1.2f * dt_sec);
    if (in.held(sdlpp::scancode::left))  m_speed = std::max(0.2f, m_speed - 1.2f * dt_sec);

    // Phase increment stepping (Authentic WAVY.PAS: add1=1, add2=-1, add3=-1)
    const int step = std::max(1, static_cast<int>(std::round(m_speed)));
    m_i1 = (m_i1 + step) % 255;
    m_i2 = (m_i2 - step + 2550) % 255;
    m_i3 = (m_i3 - step + 2550) % 255;
}

void wavy_ribbon_effect::render(const neutrino::rect& viewport) {
    uint8_t* raw = m_canvas.raw_pixels();

    // Mode 0: Authentic 1995 WAVY.PAS Single Wave
    if (m_mode == 0) {
        // Save previous wave positions for clean erasure
        m_ptab = m_ctab;

        for (int x = 0; x < 320; ++x) {
            const int raw_val = m_stab1[(x + m_i1) % 255]
                              + m_stab2[(x + m_i2) % 255]
                              + m_stab3[(x + m_i3) % 255];

            // Center around 99 with amplitude scaling
            const int scaled_val = 99 + static_cast<int>(std::round((raw_val - 99) * m_amplitude));
            const int y = std::clamp(scaled_val, 0, 199);
            m_ctab[x] = static_cast<uint8_t>(y);

            // Erase previous point, plot current point
            m_canvas.put_pixel(x, m_ptab[x], 0);
            m_canvas.put_pixel(x, m_ctab[x], m_ctab[x]);
        }
    }
    // Mode 1: 3D Undulating Silk Ribbon with Height Spans & Shading
    else if (m_mode == 1) {
        m_canvas.clear(0);

        for (int x = 0; x < 320; ++x) {
            const int raw_center = m_stab1[(x + m_i1) % 255]
                                 + m_stab2[(x + m_i2) % 255]
                                 + m_stab3[(x + m_i3) % 255];
            const int y_center = 99 + static_cast<int>(std::round((raw_center - 99) * m_amplitude));

            // Dynamic ribbon thickness simulating 3D rotation twist
            const float twist = std::sin(static_cast<float>(x) * 0.04f + m_time * 2.5f);
            const int thickness = 8 + static_cast<int>(std::abs(twist) * 18.0f);

            const int y_top = std::clamp(y_center - thickness, 0, 199);
            const int y_bot = std::clamp(y_center + thickness, 0, 199);

            // Shaded vertical scanline gradient
            const int span = y_bot - y_top;
            if (span > 0) {
                for (int y = y_top; y <= y_bot; ++y) {
                    const float factor = static_cast<float>(y - y_top) / static_cast<float>(span);
                    // Specular highlight in the middle, dark edges
                    const float intensity = std::sin(factor * 3.14159f);
                    const int col = std::clamp(static_cast<int>(intensity * 220.0f) + 30, 1, 255);
                    m_canvas.put_pixel(x, y, static_cast<uint8_t>(col));
                }
            }

            // Crisp ribbon borders
            m_canvas.put_pixel(x, y_top, 255);
            m_canvas.put_pixel(x, y_bot, 220);
        }
    }
    // Mode 2: Phosphor Wave Trail / CRT Decay
    else if (m_mode == 2) {
        // ~85% attenuation decay
        for (int i = 0; i < vga_canvas::pixel_count; ++i) {
            raw[i] = static_cast<uint8_t>((static_cast<uint32_t>(raw[i]) * 218) >> 8);
        }

        for (int x = 0; x < 320; ++x) {
            const int raw_val = m_stab1[(x + m_i1) % 255]
                              + m_stab2[(x + m_i2) % 255]
                              + m_stab3[(x + m_i3) % 255];
            const int y = std::clamp(99 + static_cast<int>(std::round((raw_val - 99) * m_amplitude)), 1, 198);

            // Glowing anti-aliased wave head
            m_canvas.put_pixel(x, y, 255);
            m_canvas.put_pixel(x, y - 1, 180);
            m_canvas.put_pixel(x, y + 1, 180);
        }
    }
    // Mode 3: Fourier Harmonic Decomposition
    else {
        m_canvas.clear(0);

        // Draw centerline
        for (int x = 0; x < 320; x += 4) {
            m_canvas.put_pixel(x, 99, 40);
        }

        for (int x = 0; x < 320; ++x) {
            // Harmonic 1 (Fundamental Sine, ω=1)
            const int y1 = std::clamp(m_stab1[(x + m_i1) % 255], 0, 199);
            m_canvas.put_pixel(x, y1, 60);

            // Harmonic 2 (Cosine, ω=2)
            const int y2 = std::clamp(99 + m_stab2[(x + m_i2) % 255], 0, 199);
            m_canvas.put_pixel(x, y2, 100);

            // Harmonic 3 (Sine, ω=2)
            const int y3 = std::clamp(99 + m_stab3[(x + m_i3) % 255], 0, 199);
            m_canvas.put_pixel(x, y3, 140);

            // Compound Interference Sum
            const int raw_val = m_stab1[(x + m_i1) % 255]
                              + m_stab2[(x + m_i2) % 255]
                              + m_stab3[(x + m_i3) % 255];
            const int y_sum = std::clamp(99 + static_cast<int>(std::round((raw_val - 99) * m_amplitude)), 0, 199);
            m_canvas.put_pixel(x, y_sum, 255);
            if (y_sum > 0) m_canvas.put_pixel(x, y_sum - 1, 230);
            if (y_sum < 199) m_canvas.put_pixel(x, y_sum + 1, 230);
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
