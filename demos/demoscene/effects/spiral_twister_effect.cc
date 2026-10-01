#include "spiral_twister_effect.hh"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace demoscene {

spiral_twister_effect::spiral_twister_effect() {
    on_enter();
}

void spiral_twister_effect::init_palette() {
    m_canvas.set_rgb(0, 4, 6, 12); // Deep midnight backdrop

    switch (m_palette_theme) {
    case 0: {
        // Amiga Classic 4-Face Copper Twister Palette (4 ramps of 64 colors)
        // Face 0: Cyan / Electric Ice Blue (1..63)
        for (int i = 1; i <= 63; ++i) {
            const float t = static_cast<float>(i) / 63.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(t * 40),
                static_cast<uint8_t>(30 + t * 180),
                static_cast<uint8_t>(80 + t * 175));
        }
        // Face 1: Neon Magenta / Hot Pink (64..127)
        for (int i = 64; i <= 127; ++i) {
            const float t = static_cast<float>(i - 64) / 63.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(80 + t * 175),
                static_cast<uint8_t>(t * 40),
                static_cast<uint8_t>(70 + t * 140));
        }
        // Face 2: Golden Amber / Solar Flame (128..191)
        for (int i = 128; i <= 191; ++i) {
            const float t = static_cast<float>(i - 128) / 63.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(100 + t * 155),
                static_cast<uint8_t>(50 + t * 175),
                static_cast<uint8_t>(t * 30));
        }
        // Face 3: Emerald Lime Green (192..255)
        for (int i = 192; i <= 255; ++i) {
            const float t = static_cast<float>(i - 192) / 63.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(20 + t * 80),
                static_cast<uint8_t>(90 + t * 165),
                static_cast<uint8_t>(30 + t * 60));
        }
        break;
    }
    case 1: {
        // Cyberpunk Synthwave: Deep Violet -> Cyan -> Pure White
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.5f) {
                const float u = t / 0.5f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(40 + u * 180),
                    static_cast<uint8_t>(u * 30),
                    static_cast<uint8_t>(80 + u * 150));
            } else {
                const float u = (t - 0.5f) / 0.5f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(220 + u * 35),
                    static_cast<uint8_t>(30 + u * 225),
                    static_cast<uint8_t>(230 + u * 25));
            }
        }
        break;
    }
    case 2: {
        // Solar Gold & Magma: Crimson -> Fiery Gold -> Blinding White
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(80 + u * 175),
                    static_cast<uint8_t>(u * 140),
                    static_cast<uint8_t>(u * 15));
            } else {
                const float u = (t - 0.6f) / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    255,
                    static_cast<uint8_t>(140 + u * 115),
                    static_cast<uint8_t>(15 + u * 240));
            }
        }
        break;
    }
    default:
        break;
    }
}

void spiral_twister_effect::on_enter() {
    init_palette();
    m_canvas.clear(0);
    m_time = 0.0f;
    m_rot_speed = 2.4f;
    m_twist_rate = 0.035f;
    m_mode = 0;
    m_palette_theme = 0;
}

void spiral_twister_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    // Cycle Display Mode
    if (in.pressed(sdlpp::scancode::space)) {
        m_mode = (m_mode + 1) % 3;
        m_canvas.clear(0);
    }

    // Cycle Color Theme
    if (in.pressed(sdlpp::scancode::c)) {
        m_palette_theme = (m_palette_theme + 1) % 3;
        init_palette();
    }

    // Twist Rate Controls
    if (in.held(sdlpp::scancode::up))   m_twist_rate = std::min(0.09f, m_twist_rate + 0.04f * dt_sec);
    if (in.held(sdlpp::scancode::down)) m_twist_rate = std::max(0.005f, m_twist_rate - 0.04f * dt_sec);

    // Rotation Speed Controls
    if (in.held(sdlpp::scancode::left))  m_rot_speed -= 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::right)) m_rot_speed += 2.0f * dt_sec;

    if (in.pressed(sdlpp::scancode::r)) {
        m_time = 0.0f;
        m_rot_speed = 2.4f;
        m_twist_rate = 0.035f;
        m_mode = 0;
        m_palette_theme = 0;
        init_palette();
        m_canvas.clear(0);
    }
}

void spiral_twister_effect::render_twister_column() {
    m_canvas.clear(0);

    constexpr float pi = std::numbers::pi_v<float>;
    constexpr float half_pi = pi * 0.5f;

    // Scanline-by-scanline 3D twister column rasterization
    for (int y = 0; y < vga_canvas::height; ++y) {
        const float fy = static_cast<float>(y);

        // Sinusoidal column centerline wobble (S-curve)
        const float cx = 160.0f + 32.0f * std::sin(fy * 0.024f + m_time * 1.8f) + 12.0f * std::cos(fy * 0.045f - m_time * 2.2f);
        const float radius = 55.0f + 16.0f * std::cos(fy * 0.032f - m_time * 1.4f);

        // Twist angle at scanline y
        const float angle = m_time * m_rot_speed + fy * m_twist_rate + 0.45f * std::sin(fy * 0.028f + m_time * 2.1f);

        // 4 corners of the twisting square column
        float vx[4], vz[4];
        for (int k = 0; k < 4; ++k) {
            const float a = angle + static_cast<float>(k) * half_pi;
            vx[k] = cx + radius * std::cos(a);
            vz[k] = radius * std::sin(a);
        }

        // Render visible faces (normals pointing towards viewer: front-facing when vx[next] > vx[k])
        for (int k = 0; k < 4; ++k) {
            const int next_k = (k + 1) & 3;
            const float x0 = vx[k];
            const float x1 = vx[next_k];

            // Backface culling: face is visible to camera if x1 > x0
            if (x1 > x0) {
                const int start_x = std::max(0, static_cast<int>(x0));
                const int end_x = std::min(vga_canvas::width - 1, static_cast<int>(x1));
                const float span = x1 - x0;
                if (span < 1e-3f) continue;

                const int base_col = (m_palette_theme == 0) ? (k * 64 + 1) : 1;
                const int col_range = (m_palette_theme == 0) ? 62 : 254;

                // Gouraud / cylindrical Lambertian shading across the face
                const float nx0 = (x0 - cx) / radius;
                const float nx1 = (x1 - cx) / radius;

                for (int x = start_x; x <= end_x; ++x) {
                    const float t = static_cast<float>(x - x0) / span;
                    const float nx = nx0 + (nx1 - nx0) * t;
                    const float nz2 = 1.0f - nx * nx;
                    const float nz = (nz2 > 0.0f) ? std::sqrt(nz2) : 0.0f;

                    // Specular highlight + diffuse lighting
                    const float shade = std::clamp(0.25f + 0.55f * nz + 0.35f * std::pow(nz, 8.0f), 0.0f, 1.0f);
                    const uint8_t color = static_cast<uint8_t>(base_col + static_cast<int>(shade * col_range));

                    m_canvas.put_pixel_fast(x, y, color);
                }
            }
        }
    }
}

void spiral_twister_effect::render_polar_spiral() {
    constexpr float pi = std::numbers::pi_v<float>;

    for (int y = 0; y < vga_canvas::height; ++y) {
        const float dy = static_cast<float>(y - 100);
        for (int x = 0; x < vga_canvas::width; ++x) {
            const float dx = static_cast<float>(x - 160);

            const euler::complex<float> z{dx, dy};
            const float r = std::sqrt(z.norm());
            const float theta = std::atan2(z.imag(), z.real());

            // Multi-arm logarithmic & Archimedean spiral formula
            const float spiral_arm = theta * (4.0f / pi) + (r * 0.12f) - (m_time * 4.5f);
            const float ripple = std::sin(spiral_arm) + 0.5f * std::sin(r * 0.08f - m_time * 3.0f);
            const float wave = 0.5f + 0.5f * std::cos(theta * 3.0f + ripple * 2.0f);

            const int col_val = std::clamp(static_cast<int>(wave * 230.0f + 20.0f), 1, 255);
            m_canvas.put_pixel_fast(x, y, static_cast<uint8_t>(col_val));
        }
    }
}

void spiral_twister_effect::render_sine_ribbon() {
    m_canvas.clear(0);

    // Amiga Sine Ribbon Scroller (SPIRAL.PAS / SCR_SPRL.PAS)
    constexpr int ribbon_thickness = 26;

    for (int x = 0; x < vga_canvas::width; ++x) {
        const float fx = static_cast<float>(x);

        // Dynamic multi-frequency sine wave path
        const float base_y = 100.0f
            + 40.0f * std::sin(fx * 0.024f + m_time * 3.2f)
            + 18.0f * std::cos(fx * 0.055f - m_time * 2.1f);

        const int cy = static_cast<int>(base_y);

        for (int dy = -ribbon_thickness / 2; dy <= ribbon_thickness / 2; ++dy) {
            const int py = cy + dy;
            if (py < 0 || py >= vga_canvas::height) continue;

            const float v = static_cast<float>(dy + ribbon_thickness / 2) / static_cast<float>(ribbon_thickness);
            const float shade = std::sin(v * std::numbers::pi_v<float>); // Cylindrical profile

            // Color bands moving along ribbon
            const int color_idx = static_cast<int>(shade * 200.0f + 40.0f);
            const uint8_t col = static_cast<uint8_t>(std::clamp(color_idx, 1, 255));

            m_canvas.put_pixel_fast(x, py, col);
        }

        // Ribbon highlight ridge
        if (cy >= 0 && cy < vga_canvas::height) {
            m_canvas.put_pixel_fast(x, cy, 255);
        }
    }
}

void spiral_twister_effect::render(const neutrino::rect& viewport) {
    switch (m_mode) {
    case 0:
        render_twister_column();
        break;
    case 1:
        render_polar_spiral();
        break;
    case 2:
        render_sine_ribbon();
        break;
    default:
        break;
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
