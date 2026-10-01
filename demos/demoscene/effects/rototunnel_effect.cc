#include "rototunnel_effect.hh"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace demoscene {

rototunnel_effect::rototunnel_effect()
    : m_texture(256 * 256, 0)
{
    generate_texture(0);
    on_enter();
}

void rototunnel_effect::generate_texture(int pattern_idx) {
    constexpr int size = 256;

    switch (pattern_idx) {
    case 0: { // Classic Demoscene Stone Checkerboard
        for (int y = 0; y < size; ++y) {
            const int ty = y / 16;
            for (int x = 0; x < size; ++x) {
                const int tx = x / 16;
                const bool check = ((tx ^ ty) & 1) == 0;
                const int grain = (x * 13 + y * 17) % 25;
                const int val = check ? (190 + grain) : (60 + grain);
                m_texture[y * size + x] = static_cast<uint8_t>(std::clamp(val, 10, 250));
            }
        }
        break;
    }
    case 1: { // Cyber Hexagon Grid
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                int val = 20;
                // Hexagonal grid approximation
                if (x % 32 < 2 || y % 32 < 2 || ((x + y) % 32 < 2) || ((x - y + 256) % 32 < 2)) {
                    val = 220;
                } else if ((x % 32 == 16) && (y % 32 == 16)) {
                    val = 180;
                }
                m_texture[y * size + x] = static_cast<uint8_t>(val);
            }
        }
        break;
    }
    case 2: { // Bio-Organic Ribbed Tunnel
        for (int y = 0; y < size; ++y) {
            const float fy = static_cast<float>(y);
            for (int x = 0; x < size; ++x) {
                const float fx = static_cast<float>(x);
                const float rib = std::sin(fy * 0.15f) + 0.5f * std::cos(fx * 0.18f);
                const int val = static_cast<int>(130.0f + rib * 90.0f);
                m_texture[y * size + x] = static_cast<uint8_t>(std::clamp(val, 15, 245));
            }
        }
        break;
    }
    default:
        break;
    }
}

void rototunnel_effect::init_palette() {
    m_canvas.set_rgb(0, 4, 4, 8); // Tunnel vanishing depth black

    switch (m_color_theme) {
    case 0: { // Amber Gold / Solar Flame: Deep mahogany -> Fiery gold -> Blinding white
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(60 + u * 185),
                    static_cast<uint8_t>(20 + u * 120),
                    static_cast<uint8_t>(u * 20));
            } else {
                const float u = (t - 0.6f) / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    255,
                    static_cast<uint8_t>(140 + u * 115),
                    static_cast<uint8_t>(20 + u * 235));
            }
        }
        break;
    }
    case 1: { // Electric Azure / Ice Cyan: Deep navy -> Vibrant turquoise -> Arctic white
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(u * 30),
                    static_cast<uint8_t>(40 + u * 170),
                    static_cast<uint8_t>(70 + u * 185));
            } else {
                const float u = (t - 0.6f) / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(30 + u * 225),
                    static_cast<uint8_t>(210 + u * 45),
                    255);
            }
        }
        break;
    }
    case 2: { // Toxic Neon / Matrix Green: Dark moss -> Electric green -> Solar yellow
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(u * 40),
                    static_cast<uint8_t>(40 + u * 190),
                    static_cast<uint8_t>(10 + u * 40));
            } else {
                const float u = (t - 0.6f) / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(40 + u * 215),
                    255,
                    static_cast<uint8_t>(50 + u * 190));
            }
        }
        break;
    }
    default:
        break;
    }
}

void rototunnel_effect::on_enter() {
    init_palette();
    m_canvas.clear(0);
    m_cam_x = 160.0f;
    m_cam_y = 100.0f;
    m_speed = 120.0f;
    m_roll_speed = 0.5f;
    m_tunnel_pos = 0.0f;
    m_tunnel_rot = 0.0f;
    m_time = 0.0f;
    m_pattern_idx = 0;
    m_color_theme = 0;
    generate_texture(0);
}

void rototunnel_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    m_tunnel_pos += m_speed * dt_sec;
    m_tunnel_rot += m_roll_speed * dt_sec;

    // Lissajous camera sway through tunnel interior
    m_cam_x = 160.0f + 48.0f * std::cos(m_time * 1.3f);
    m_cam_y = 100.0f + 32.0f * std::sin(m_time * 1.9f);

    // Controls
    if (in.held(sdlpp::scancode::up))   m_speed = std::min(300.0f, m_speed + 80.0f * dt_sec);
    if (in.held(sdlpp::scancode::down)) m_speed = std::max(20.0f, m_speed - 80.0f * dt_sec);
    if (in.held(sdlpp::scancode::left))  m_roll_speed -= 1.0f * dt_sec;
    if (in.held(sdlpp::scancode::right)) m_roll_speed += 1.0f * dt_sec;

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

void rototunnel_effect::render(const neutrino::rect& viewport) {
    constexpr int w = vga_canvas::width;
    constexpr int h = vga_canvas::height;

    uint8_t* raw = m_canvas.raw_pixels();
    const uint8_t* tex = m_texture.data();

    constexpr float two_pi = 2.0f * std::numbers::pi_v<float>;
    constexpr float angle_scale = 256.0f / two_pi;
    constexpr float tunnel_depth_c = 4200.0f;

    const float cx = m_cam_x;
    const float cy = m_cam_y;

    for (int y = 0; y < h; ++y) {
        const float dy = static_cast<float>(y) - cy;
        const float dy2 = dy * dy;

        uint8_t* row = raw + y * w;

        for (int x = 0; x < w; ++x) {
            const float dx = static_cast<float>(x) - cx;
            const float r2 = dx * dx + dy2;

            if (r2 < 4.0f) {
                row[x] = 0; // Vanishing point center
                continue;
            }

            const float r = std::sqrt(r2);
            const float angle = std::atan2(dy, dx);

            // Polar coordinates
            const float u = (angle + std::numbers::pi_v<float>) * angle_scale + m_tunnel_rot * 40.0f;
            const float v = (tunnel_depth_c / r) + m_tunnel_pos;

            const int iu = static_cast<int>(u) & 255;
            const int iv = static_cast<int>(v) & 255;

            uint8_t col = tex[(iv << 8) | iu];

            // Exponential depth fog towards center
            const float fog = std::clamp((r - 6.0f) / 55.0f, 0.0f, 1.0f);
            col = static_cast<uint8_t>(static_cast<float>(col) * fog);

            row[x] = col;
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
