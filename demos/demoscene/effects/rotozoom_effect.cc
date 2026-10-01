#include "rotozoom_effect.hh"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace demoscene {

rotozoom_effect::rotozoom_effect()
    : m_texture(256 * 256, 0)
{
    generate_texture(0);
    on_enter();
}

void rotozoom_effect::generate_texture(int pattern_idx) {
    constexpr int size = 256;

    switch (pattern_idx) {
    case 0: { // Checkerboard Rosette: 16x16 alternating tiles with concentric radial rosette
        for (int y = 0; y < size; ++y) {
            const float dy = static_cast<float>(y - 128);
            const int ty = y / 16;
            for (int x = 0; x < size; ++x) {
                const float dx = static_cast<float>(x - 128);
                const int tx = x / 16;
                const bool check = ((tx ^ ty) & 1) == 0;

                const float r = std::sqrt(dx * dx + dy * dy);
                const float angle = std::atan2(dy, dx);
                const float rosette = std::cos(angle * 6.0f + r * 0.15f);

                int val = check ? 180 : 60;
                val += static_cast<int>(rosette * 40.0f);
                m_texture[y * size + x] = static_cast<uint8_t>(std::clamp(val, 10, 250));
            }
        }
        break;
    }
    case 1: { // Cyber Circuit: Grid traces, chips, and glowing circuit nodes
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                int val = 25;
                if (x % 32 == 0 || y % 32 == 0 || ((x + y) % 48 == 0)) val = 120;
                if ((x % 32 < 6) && (y % 32 < 6)) val = 240; // Contact pads
                if (x >= 40 && x <= 90 && y >= 40 && y <= 90) val = 190; // IC chip
                if (x >= 150 && x <= 220 && y >= 140 && y <= 210) val = 210;

                m_texture[y * size + x] = static_cast<uint8_t>(val);
            }
        }
        break;
    }
    case 2: { // Geometric Greek Labyrinth / Concentric Diamond Maze
        for (int y = 0; y < size; ++y) {
            const int dy = std::abs(y - 128);
            for (int x = 0; x < size; ++x) {
                const int dx = std::abs(x - 128);
                const int diamond = dx + dy;
                const int ring = diamond % 24;
                const int val = (ring < 12) ? (40 + ring * 12) : (220 - (ring - 12) * 12);
                m_texture[y * size + x] = static_cast<uint8_t>(std::clamp(val, 15, 245));
            }
        }
        break;
    }
    default:
        break;
    }
}

void rotozoom_effect::init_palette() {
    m_canvas.set_rgb(0, 8, 8, 14);

    switch (m_color_theme) {
    case 0: { // Copper Rainbow Spectrum: Deep mahogany -> Amber gold -> Cyan highlight
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.5f) {
                const float u = t / 0.5f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(60 + u * 195),
                    static_cast<uint8_t>(20 + u * 120),
                    static_cast<uint8_t>(u * 30));
            } else {
                const float u = (t - 0.5f) / 0.5f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(255 - u * 120),
                    static_cast<uint8_t>(140 + u * 100),
                    static_cast<uint8_t>(30 + u * 225));
            }
        }
        break;
    }
    case 1: { // Synthwave Neon: Deep purple -> Electric magenta -> Hot neon cyan
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.5f) {
                const float u = t / 0.5f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(50 + u * 190),
                    static_cast<uint8_t>(u * 20),
                    static_cast<uint8_t>(80 + u * 120));
            } else {
                const float u = (t - 0.5f) / 0.5f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(240 - u * 200),
                    static_cast<uint8_t>(20 + u * 220),
                    255);
            }
        }
        break;
    }
    case 2: { // Matrix Emerald Lime: Dark moss -> Vivid jade -> Incandescent toxic yellow
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(u * 30),
                    static_cast<uint8_t>(40 + u * 190),
                    static_cast<uint8_t>(20 + u * 40));
            } else {
                const float u = (t - 0.6f) / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(30 + u * 225),
                    255,
                    static_cast<uint8_t>(60 + u * 180));
            }
        }
        break;
    }
    default:
        break;
    }
}

void rotozoom_effect::on_enter() {
    init_palette();
    m_canvas.clear(0);
    m_angle = 0.0f;
    m_rot_speed = 0.8f;
    m_zoom = 1.0f;
    m_pos_u = 128.0f;
    m_pos_v = 128.0f;
    m_time = 0.0f;
    m_pattern_idx = 0;
    m_color_theme = 0;
    generate_texture(0);
}

void rotozoom_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    // Continuous rotation and sinusoidal zoom oscillation
    m_angle += m_rot_speed * dt_sec;
    const float dyn_zoom = m_zoom * (1.2f + 0.5f * std::sin(m_time * 1.5f));

    // Pan across texture
    m_pos_u += 45.0f * dt_sec * std::cos(m_angle * 0.5f);
    m_pos_v += 35.0f * dt_sec * std::sin(m_angle * 0.7f);

    // Controls
    if (in.held(sdlpp::scancode::left))  m_rot_speed -= 1.5f * dt_sec;
    if (in.held(sdlpp::scancode::right)) m_rot_speed += 1.5f * dt_sec;
    if (in.held(sdlpp::scancode::up))    m_zoom = std::min(4.0f, m_zoom + 1.2f * dt_sec);
    if (in.held(sdlpp::scancode::down))  m_zoom = std::max(0.2f, m_zoom - 1.2f * dt_sec);

    if (in.pressed(sdlpp::scancode::space)) {
        m_pattern_idx = (m_pattern_idx + 1) % 3;
        generate_texture(m_pattern_idx);
    }

    if (in.pressed(sdlpp::scancode::c)) {
        m_color_theme = (m_color_theme + 1) % 3;
        init_palette();
    }

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }
}

void rotozoom_effect::render(const neutrino::rect& viewport) {
    constexpr int w = vga_canvas::width;
    constexpr int h = vga_canvas::height;

    uint8_t* raw = m_canvas.raw_pixels();
    const uint8_t* tex = m_texture.data();

    const float dyn_zoom = m_zoom * (1.2f + 0.5f * std::sin(m_time * 1.5f));
    const float inv_zoom = 1.0f / dyn_zoom;

    const float cos_a = std::cos(m_angle) * inv_zoom;
    const float sin_a = std::sin(m_angle) * inv_zoom;

    // Scanline differential steps
    const float du_x = cos_a;
    const float dv_x = sin_a;
    const float du_y = -sin_a;
    const float dv_y = cos_a;

    constexpr float half_w = static_cast<float>(w) * 0.5f;
    constexpr float half_h = static_cast<float>(h) * 0.5f;

    for (int y = 0; y < h; ++y) {
        const float dy = static_cast<float>(y) - half_h;
        float u = m_pos_u + (-half_w * du_x + dy * du_y);
        float v = m_pos_v + (-half_w * dv_x + dy * dv_y);

        uint8_t* row = raw + y * w;

        for (int x = 0; x < w; ++x) {
            const int iu = static_cast<int>(u) & 255;
            const int iv = static_cast<int>(v) & 255;

            row[x] = tex[(iv << 8) | iu];

            u += du_x;
            v += dv_x;
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
