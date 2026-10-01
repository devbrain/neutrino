#include "sinmap_effect.hh"

#include <algorithm>
#include <cmath>

namespace demoscene {

sinmap_effect::sinmap_effect()
    : m_texture(tex_w * tex_h, 0)
{
    on_enter();
}

void sinmap_effect::init_palette() {
    // 0: Deep backdrop
    m_canvas.set_rgb(0, 10, 12, 22);

    // 1..32: Red ramp for Dutch Flag (shadow to specular)
    for (int i = 1; i <= 32; ++i) {
        const float t = static_cast<float>(i) / 32.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(100 + t * 155),
            static_cast<uint8_t>(t * 40),
            static_cast<uint8_t>(t * 40));
    }

    // 33..64: White / Pearl ramp for Dutch Flag
    for (int i = 33; i <= 64; ++i) {
        const float t = static_cast<float>(i - 33) / 31.0f;
        const uint8_t v = static_cast<uint8_t>(130 + t * 125);
        m_canvas.set_rgb(static_cast<uint8_t>(i), v, v, v);
    }

    // 65..96: Cobalt Blue ramp for Dutch Flag
    for (int i = 65; i <= 96; ++i) {
        const float t = static_cast<float>(i - 65) / 31.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(t * 40),
            static_cast<uint8_t>(40 + t * 100),
            static_cast<uint8_t>(120 + t * 135));
    }

    // 97..128: Cyan / Violet ramp for Checkerboard
    for (int i = 97; i <= 128; ++i) {
        const float t = static_cast<float>(i - 97) / 31.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(40 + t * 80),
            static_cast<uint8_t>(160 + t * 95),
            static_cast<uint8_t>(200 + t * 55));
    }

    // 129..160: Magenta / Purple ramp for Checkerboard
    for (int i = 129; i <= 160; ++i) {
        const float t = static_cast<float>(i - 129) / 31.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(180 + t * 75),
            static_cast<uint8_t>(20 + t * 40),
            static_cast<uint8_t>(140 + t * 100));
    }

    // 161..192: Golden Sunburst ramp
    for (int i = 161; i <= 192; ++i) {
        const float t = static_cast<float>(i - 161) / 31.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(200 + t * 55),
            static_cast<uint8_t>(140 + t * 100),
            static_cast<uint8_t>(20 + t * 40));
    }
}

void sinmap_effect::generate_texture_pattern() {
    for (int y = 0; y < tex_h; ++y) {
        for (int x = 0; x < tex_w; ++x) {
            const size_t idx = static_cast<size_t>(y * tex_w + x);

            switch (m_pattern_mode) {
            case 0: {
                // Authentic 1994 Dutch Flag: Red, White, Blue horizontal tri-band
                if (y < tex_h / 3) {
                    m_texture[idx] = 16; // Red band base
                } else if (y < (2 * tex_h) / 3) {
                    m_texture[idx] = 48; // White band base
                } else {
                    m_texture[idx] = 80; // Blue band base
                }
                break;
            }
            case 1: {
                // Demoscene Cyan / Purple Checkerboard
                const bool check = (((x / 16) ^ (y / 16)) & 1) != 0;
                m_texture[idx] = check ? 112 : 144;
                break;
            }
            case 2: {
                // Concentric Ripple Bullseye Rings
                const float dx = static_cast<float>(x - tex_w / 2);
                const float dy = static_cast<float>(y - tex_h / 2);
                const float r = std::sqrt(dx * dx + dy * dy);
                const bool ring = (static_cast<int>(r / 8.0f) & 1) != 0;
                m_texture[idx] = ring ? 176 : 112;
                break;
            }
            default:
                break;
            }
        }
    }
}

void sinmap_effect::on_enter() {
    init_palette();
    generate_texture_pattern();
    m_time = 0.0f;
    m_amp_x = 18.0f;
    m_amp_y = 14.0f;
    m_freq = 1.0f;
}

void sinmap_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    // Interactive Amplitude Controls
    if (in.held(sdlpp::scancode::up))   m_amp_x = std::min(40.0f, m_amp_x + 15.0f * dt_sec);
    if (in.held(sdlpp::scancode::down)) m_amp_x = std::max(2.0f,  m_amp_x - 15.0f * dt_sec);
    m_amp_y = m_amp_x * 0.75f;

    // Interactive Wave Frequency Controls
    if (in.held(sdlpp::scancode::left))  m_freq = std::max(0.4f, m_freq - 0.6f * dt_sec);
    if (in.held(sdlpp::scancode::right)) m_freq = std::min(2.5f, m_freq + 0.6f * dt_sec);

    // Switch Texture Pattern
    if (in.pressed(sdlpp::scancode::space)) {
        m_pattern_mode = (m_pattern_mode + 1) % 3;
        generate_texture_pattern();
    }

    if (in.pressed(sdlpp::scancode::r)) {
        m_amp_x = 18.0f;
        m_amp_y = 14.0f;
        m_freq = 1.0f;
        m_pattern_mode = 0;
        generate_texture_pattern();
    }
}

void sinmap_effect::render(const neutrino::rect& viewport) {
    m_canvas.clear(0);

    constexpr int screen_center_x = vga_canvas::width / 2;
    constexpr int screen_center_y = vga_canvas::height / 2;

    const int offset_x = screen_center_x - tex_w / 2;
    const int offset_y = screen_center_y - tex_h / 2;

    // Inverse mapping: For each screen pixel inside bounding area,
    // evaluate the sinusoidal displacement field and sample the texture.
    for (int sy = 0; sy < vga_canvas::height; ++sy) {
        const float fsy = static_cast<float>(sy);

        for (int sx = 0; sx < vga_canvas::width; ++sx) {
            const float fsx = static_cast<float>(sx);

            // Compound sinusoidal 2D displacement
            const float wave1 = std::sin(fsy * 0.045f * m_freq + m_time * 2.8f);
            const float wave2 = std::cos(fsx * 0.040f * m_freq - m_time * 2.2f);
            const float wave3 = std::sin((fsx + fsy) * 0.025f * m_freq + m_time * 1.5f);

            const float dx = m_amp_x * wave1 + (m_amp_x * 0.35f) * wave2;
            const float dy = m_amp_y * wave2 + (m_amp_y * 0.30f) * wave3;

            // Map screen coords back to texture coords (u, v)
            const int u = static_cast<int>(fsx - static_cast<float>(offset_x) - dx);
            const int v = static_cast<int>(fsy - static_cast<float>(offset_y) - dy);

            if (u >= 0 && u < tex_w && v >= 0 && v < tex_h) {
                const uint8_t base_color = m_texture[static_cast<size_t>(v * tex_w + u)];

                // Calculate surface normal / silk specular highlight from wave derivative
                const float slope = std::cos(fsy * 0.045f * m_freq + m_time * 2.8f);
                const int shade_offset = static_cast<int>(slope * 12.0f);

                const uint8_t final_color = static_cast<uint8_t>(
                    std::clamp(static_cast<int>(base_color) + shade_offset, 1, 255)
                );

                m_canvas.put_pixel_fast(sx, sy, final_color);
            }
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
