#include "xor_patterns_effect.hh"

#include <algorithm>
#include <cmath>

namespace demoscene {

xor_patterns_effect::xor_patterns_effect() {
    on_enter();
}

void xor_patterns_effect::on_enter() {
    m_canvas.clear(0);
    init_palette();
    m_scale = 1.0f;
    m_pan_x = 0.0f;
    m_pan_y = 0.0f;
    m_time = 0.0f;
}

void xor_patterns_effect::init_palette() {
    m_canvas.set_rgb_6bit(0, 0, 0, 0);

    switch (m_theme) {
    case 0: { // Neon Amethyst / Violet Pink (Authentic xorcircles style)
        for (int i = 0; i < 256; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(140.0f + std::sin(t * 6.283f) * 110.0f),
                static_cast<uint8_t>(30.0f + std::cos(t * 3.1415f) * 30.0f),
                static_cast<uint8_t>(180.0f + std::cos(t * 6.283f) * 75.0f));
        }
        break;
    }
    case 1: { // Amber Cyberpunk Flame
        for (int i = 0; i < 256; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(std::min(255.0f, t * 300.0f)),
                static_cast<uint8_t>(t * t * 210.0f),
                static_cast<uint8_t>(t * t * t * 80.0f));
        }
        break;
    }
    case 2: { // Laser Cyan Matrix
        for (int i = 0; i < 256; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(t * t * 60.0f),
                static_cast<uint8_t>(50.0f + t * 205.0f),
                static_cast<uint8_t>(120.0f + t * 135.0f));
        }
        break;
    }
    case 3: { // Monochromatic Phosphor Green
        for (int i = 0; i < 256; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(t * 40.0f),
                static_cast<uint8_t>(t * 255.0f),
                static_cast<uint8_t>(t * 40.0f));
        }
        break;
    }
    }
}

void xor_patterns_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    if (in.pressed(sdlpp::scancode::space)) {
        m_mode = (m_mode + 1) % 3;
    }

    if (in.pressed(sdlpp::scancode::c)) {
        m_theme = (m_theme + 1) % 4;
        init_palette();
    }

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }

    if (in.held(sdlpp::scancode::equals) || in.held(sdlpp::scancode::kp_plus)) {
        m_scale = std::min(4.0f, m_scale + 1.2f * dt_sec);
    }
    if (in.held(sdlpp::scancode::minus) || in.held(sdlpp::scancode::kp_minus)) {
        m_scale = std::max(0.25f, m_scale - 1.2f * dt_sec);
    }

    if (in.held(sdlpp::scancode::left))  m_pan_x -= 120.0f * dt_sec;
    if (in.held(sdlpp::scancode::right)) m_pan_x += 120.0f * dt_sec;
    if (in.held(sdlpp::scancode::up))    m_pan_y -= 100.0f * dt_sec;
    if (in.held(sdlpp::scancode::down))  m_pan_y += 100.0f * dt_sec;
}

void xor_patterns_effect::render(const neutrino::rect& viewport) {
    uint8_t* raw = m_canvas.raw_pixels();

    // Mode 0: Concentric Orbital XOR Circles (Authentic xorcircles.cpp)
    if (m_mode == 0) {
        const float frame = m_time * 1.5f;
        const float cx1 = 160.0f + 85.0f * std::cos(frame);
        const float cy1 = 100.0f + 50.0f * std::sin(frame * 3.3f);
        const float cx2 = 160.0f + 85.0f * std::cos(frame + 1.66f);
        const float cy2 = 100.0f + 50.0f * std::sin((frame + 1.66f) * 3.3f);

        for (int y = 0; y < vga_canvas::height; ++y) {
            const float dy1 = static_cast<float>(y) - cy1;
            const float dy2 = static_cast<float>(y) - cy2;
            const float dy1_sq = dy1 * dy1;
            const float dy2_sq = dy2 * dy2;
            uint8_t* row = raw + y * vga_canvas::width;

            for (int x = 0; x < vga_canvas::width; ++x) {
                const float dx1 = static_cast<float>(x) - cx1;
                const float dx2 = static_cast<float>(x) - cx2;

                const int r1 = static_cast<int>(std::sqrt(dx1 * dx1 + dy1_sq) * (1.6f * m_scale));
                const int r2 = static_cast<int>(std::sqrt(dx2 * dx2 + dy2_sq) * (1.6f * m_scale));

                row[x] = static_cast<uint8_t>((r1 ^ r2) & 0xFF);
            }
        }
    }
    // Mode 1: Classic Sierpinski Fractal Carpet (x ⊕ y)
    else if (m_mode == 1) {
        const float t_shift = m_time * 30.0f;
        const float scale = m_scale;
        const float ox = m_pan_x + t_shift;
        const float oy = m_pan_y + t_shift * 0.5f;

        for (int y = 0; y < vga_canvas::height; ++y) {
            const int iy = static_cast<int>((static_cast<float>(y) + oy) * scale);
            uint8_t* row = raw + y * vga_canvas::width;

            for (int x = 0; x < vga_canvas::width; ++x) {
                const int ix = static_cast<int>((static_cast<float>(x) + ox) * scale);
                row[x] = static_cast<uint8_t>((ix ^ iy) & 0xFF);
            }
        }
    }
    // Mode 2: Multi-Layer Bitwise Automata Mandala
    else {
        const int shift_t = static_cast<int>(m_time * 45.0f);
        const float scale = m_scale;
        const float ox = m_pan_x;
        const float oy = m_pan_y;

        for (int y = 0; y < vga_canvas::height; ++y) {
            const int iy = static_cast<int>((static_cast<float>(y) + oy) * scale);
            uint8_t* row = raw + y * vga_canvas::width;

            for (int x = 0; x < vga_canvas::width; ++x) {
                const int ix = static_cast<int>((static_cast<float>(x) + ox) * scale);

                const int p1 = (ix + shift_t) ^ iy;
                const int p2 = ix ^ (iy + shift_t);
                const int prod = (ix * iy) >> 7;

                const int result = (p1 ^ p2 ^ prod) & 0xFF;
                row[x] = static_cast<uint8_t>(result);
            }
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
