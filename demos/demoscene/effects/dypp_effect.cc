#include "dypp_effect.hh"

#include <algorithm>
#include <cmath>
#include <onyx_font/bios_font.hh>

namespace demoscene {

dypp_effect::dypp_effect() {
    m_messages = {
        "THIS THING IS CALLED A DYPP: DIFFERENT-Y-PIXEL-POSITION... UH HUH HUH HUH M... HEY BEAVIS, YOU SAID 'PIXEL'!           ",
        "GREETINGS TO ALL DEMOMAKERS! NEUTRINO RECONSTRUCTION OF BAS VAN GAALEN'S 1994 DOS DEMOS!           ",
        "COMPOUND SINE WAVES + 256 VGA PALETTE + 8x8 BIOS FONT = THE 90s DEMOSCENE MAGIC!           "
    };

    // Initialize 60 starry backdrop particles
    for (int i = 0; i < 60; ++i) {
        const float sx = static_cast<float>((i * 73 + 19) % vga_canvas::width);
        const float sy = static_cast<float>((i * 47 + 11) % vga_canvas::height);
        const float spd = 20.0f + static_cast<float>((i % 3) * 25);
        const uint8_t col = static_cast<uint8_t>(1 + (i % 5));
        m_stars.push_back({sx, sy, spd, col});
    }

    on_enter();
}

void dypp_effect::init_palette() {
    // 0: Deep space background
    m_canvas.set_rgb(0, 4, 4, 12);

    // 1..5: Star brightness levels
    m_canvas.set_rgb(1, 40,  40,  70);
    m_canvas.set_rgb(2, 80,  90, 130);
    m_canvas.set_rgb(3, 140, 150, 190);
    m_canvas.set_rgb(4, 200, 210, 240);
    m_canvas.set_rgb(5, 255, 255, 255);

    // 6: Ribbon shadow
    m_canvas.set_rgb(6, 12, 10, 25);

    // 10..34: 24-level vertical ribbon gradient (Crest specular to deep violet base)
    for (int i = 0; i < 24; ++i) {
        const float t = static_cast<float>(i) / 23.0f;
        uint8_t r = 0, g = 0, b = 0;
        if (t < 0.25f) {
            // Blinding white-yellow crest
            const float u = t / 0.25f;
            r = 255;
            g = static_cast<uint8_t>(255 - u * 50);
            b = static_cast<uint8_t>(220 * (1.0f - u));
        } else if (t < 0.55f) {
            // Hot orange-gold to magenta
            const float u = (t - 0.25f) / 0.30f;
            r = static_cast<uint8_t>(255 - u * 30);
            g = static_cast<uint8_t>(205 * (1.0f - u) + 20 * u);
            b = static_cast<uint8_t>(u * 160);
        } else {
            // Magenta to deep neon violet
            const float u = (t - 0.55f) / 0.45f;
            r = static_cast<uint8_t>(225 * (1.0f - u) + 60 * u);
            g = static_cast<uint8_t>(20 * (1.0f - u));
            b = static_cast<uint8_t>(160 + u * 95);
        }
        m_canvas.set_rgb(static_cast<uint8_t>(10 + i), r, g, b);
    }
}

void dypp_effect::on_enter() {
    init_palette();
    m_scroll_pos = 0.0f;
    m_scroll_speed = 90.0f;
    m_time = 0.0f;
    m_amp1 = 28.0f;
    m_amp2 = 14.0f;
}

void dypp_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    // Advance horizontal text scroll
    m_scroll_pos += m_scroll_speed * dt_sec;

    // Starfield drift
    for (auto& s : m_stars) {
        s.x -= s.speed * dt_sec;
        if (s.x < 0.0f) s.x += static_cast<float>(vga_canvas::width);
    }

    // Interactive controls
    if (in.pressed(sdlpp::scancode::up))   m_amp1 = std::min(45.0f, m_amp1 + 4.0f);
    if (in.pressed(sdlpp::scancode::down)) m_amp1 = std::max(5.0f,  m_amp1 - 4.0f);

    if (in.held(sdlpp::scancode::left))  m_scroll_speed = std::max(30.0f, m_scroll_speed - 40.0f * dt_sec);
    if (in.held(sdlpp::scancode::right)) m_scroll_speed = std::min(220.0f, m_scroll_speed + 40.0f * dt_sec);

    if (in.pressed(sdlpp::scancode::space)) {
        m_current_text_idx = (m_current_text_idx + 1) % m_messages.size();
        m_scroll_pos = 0.0f;
    }

    if (in.pressed(sdlpp::scancode::r)) {
        m_scroll_pos = 0.0f;
        m_scroll_speed = 90.0f;
        m_amp1 = 28.0f;
        m_amp2 = 14.0f;
    }
}

void dypp_effect::render(const neutrino::rect& viewport) {
    // 1. Clear canvas and plot stars
    m_canvas.clear(0);

    for (const auto& s : m_stars) {
        m_canvas.put_pixel(static_cast<int>(s.x), static_cast<int>(s.y), s.color);
    }

    const auto& font = onyx_font::bios_font_8x8();
    const auto& msg = m_messages[m_current_text_idx];
    const int total_pixel_width = static_cast<int>(msg.size() * 16); // 2x scaled character width (16 px)

    constexpr int glyph_scale_y = 3;  // 8x8 font scaled 3x vertically = 24 pixels tall ribbon
    constexpr int ribbon_height = 8 * glyph_scale_y;
    constexpr float mid_y = 100.0f;

    // 2. Render DYPP Sine Wave Ribbon per column
    for (int x = 0; x < vga_canvas::width; ++x) {
        // Compound harmonic vertical sine displacement
        const float fx = static_cast<float>(x);
        const float wave_y = mid_y
            + m_amp1 * std::sin(fx * 0.024f + m_time * 2.5f)
            + m_amp2 * std::cos(fx * 0.045f - m_time * 1.6f);

        const int col_y = static_cast<int>(wave_y - static_cast<float>(ribbon_height) * 0.5f);

        // Find which character and glyph column correspond to this screen column
        const int global_x = (static_cast<int>(m_scroll_pos) + x) % total_pixel_width;
        const size_t char_idx = static_cast<size_t>(global_x / 16);
        const int char_local_x = (global_x % 16) / 2; // Unscale 2x horizontally

        const char c = msg[char_idx];
        const auto glyph = font.get_glyph(static_cast<uint8_t>(c));

        // Draw Ribbon Drop Shadow (shifted by +5, +5)
        for (int gy = 0; gy < 8; ++gy) {
            if (glyph.pixel(static_cast<uint16_t>(char_local_x), static_cast<uint16_t>(gy))) {
                for (int sy = 0; sy < glyph_scale_y; ++sy) {
                    m_canvas.put_pixel(x + 5, col_y + gy * glyph_scale_y + sy + 5, 6);
                }
            }
        }

        // Draw Ribbon Column with vertical gradient shading
        for (int gy = 0; gy < 8; ++gy) {
            if (glyph.pixel(static_cast<uint16_t>(char_local_x), static_cast<uint16_t>(gy))) {
                for (int sy = 0; sy < glyph_scale_y; ++sy) {
                    const int ribbon_y_idx = gy * glyph_scale_y + sy;
                    const uint8_t color_idx = static_cast<uint8_t>(10 + ribbon_y_idx);
                    m_canvas.put_pixel(x, col_y + ribbon_y_idx, color_idx);
                }
            }
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
