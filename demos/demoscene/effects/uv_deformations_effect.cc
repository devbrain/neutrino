#include "uv_deformations_effect.hh"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>

namespace demoscene {

uv_deformations_effect::uv_deformations_effect() {
    init_palette();
    generate_textures();
    on_enter();
}

void uv_deformations_effect::init_palette() {
    m_canvas.set_rgb(0, 0, 0, 0); // Black

    // Bank 0 (1..63): Gold / Amber Brick Gradient for Texture 0
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        const uint8_t r = static_cast<uint8_t>(std::min(255.0f, t * 290.0f));
        const uint8_t g = static_cast<uint8_t>(t * 210.0f);
        const uint8_t b = static_cast<uint8_t>(t * t * 110.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
    }

    // Bank 1 (64..127): Cyber Cyan Neon Gradient for Texture 1
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        const uint8_t r = static_cast<uint8_t>(t * t * 70.0f);
        const uint8_t g = static_cast<uint8_t>(30.0f + t * 225.0f);
        const uint8_t b = static_cast<uint8_t>(100.0f + t * 155.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(64 + i), r, g, b);
    }

    // Bank 2 (128..191): Psychedelic Magenta / Purple Gradient for Texture 2
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        const uint8_t r = static_cast<uint8_t>(60.0f + t * 195.0f);
        const uint8_t g = static_cast<uint8_t>(t * t * 90.0f);
        const uint8_t b = static_cast<uint8_t>(std::min(255.0f, 110.0f + t * 145.0f));
        m_canvas.set_rgb(static_cast<uint8_t>(128 + i), r, g, b);
    }

    // Bank 3 (192..255): Chrome / White Ramp
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        const uint8_t val = static_cast<uint8_t>(t * 255.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(192 + i), val, val, val);
    }
}

void uv_deformations_effect::generate_textures() {
    // ------------------------------------------------------------------------
    // Texture 0: Aztec Stone Brick
    // ------------------------------------------------------------------------
    for (int y = 0; y < tex_size; ++y) {
        for (int x = 0; x < tex_size; ++x) {
            const int brick_row = y / 16;
            const int offset_x = (brick_row % 2 == 0) ? 0 : 16;
            const int bx = (x + offset_x) % 32;
            const int by = y % 16;

            const bool mortar = (bx == 0 || by == 0);
            const uint8_t col = mortar ? 15 : static_cast<uint8_t>(40 + ((x ^ y) % 20));
            m_textures[0][y * tex_size + x] = col;
        }
    }

    // ------------------------------------------------------------------------
    // Texture 1: Demoscene Checkerboard
    // ------------------------------------------------------------------------
    for (int y = 0; y < tex_size; ++y) {
        for (int x = 0; x < tex_size; ++x) {
            const bool check = (((x / 8) ^ (y / 8)) & 1) != 0;
            const uint8_t col = check ? 120 : 75;
            m_textures[1][y * tex_size + x] = col;
        }
    }

    // ------------------------------------------------------------------------
    // Texture 2: Cyber Circuit Lattice
    // ------------------------------------------------------------------------
    for (int y = 0; y < tex_size; ++y) {
        for (int x = 0; x < tex_size; ++x) {
            const bool trace = (x % 16 == 0 || y % 16 == 0 || (x + y) % 32 == 0);
            const uint8_t col = trace ? 185 : 135;
            m_textures[2][y * tex_size + x] = col;
        }
    }
}

void uv_deformations_effect::on_enter() {
    m_canvas.clear(0);
    m_scroll_u = 0.0f;
    m_scroll_v = 0.0f;
    m_fly_speed = 1.0f;
    m_pan_x = 0.0f;
    m_pan_y = 0.0f;
    m_time = 0.0f;
}

void uv_deformations_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    if (in.pressed(sdlpp::scancode::space)) {
        m_deform_type = static_cast<DeformType>((static_cast<int>(m_deform_type) + 1) % static_cast<int>(DeformType::Count));
    }

    if (in.pressed(sdlpp::scancode::t)) {
        m_texture_idx = (m_texture_idx + 1) % num_textures;
    }

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }

    if (in.held(sdlpp::scancode::equals) || in.held(sdlpp::scancode::kp_plus)) {
        m_fly_speed = std::min(3.0f, m_fly_speed + 1.0f * dt_sec);
    }
    if (in.held(sdlpp::scancode::minus) || in.held(sdlpp::scancode::kp_minus)) {
        m_fly_speed = std::max(0.1f, m_fly_speed - 1.0f * dt_sec);
    }

    if (in.held(sdlpp::scancode::left))  m_pan_x -= 0.8f * dt_sec;
    if (in.held(sdlpp::scancode::right)) m_pan_x += 0.8f * dt_sec;
    if (in.held(sdlpp::scancode::up))    m_pan_y -= 0.8f * dt_sec;
    if (in.held(sdlpp::scancode::down))  m_pan_y += 0.8f * dt_sec;

    m_scroll_v += 35.0f * m_fly_speed * dt_sec;
    m_scroll_u += 12.0f * m_fly_speed * dt_sec;
}

void uv_deformations_effect::render(const neutrino::rect& viewport) {
    uint8_t* raw = m_canvas.raw_pixels();
    constexpr int w = vga_canvas::width;
    constexpr int h = vga_canvas::height;

    const uint8_t* tex = m_textures[m_texture_idx].data();
    constexpr float pi = static_cast<float>(std::numbers::pi);

    for (int y = 0; y < h; ++y) {
        const float py = (static_cast<float>(y) - 100.0f) / 100.0f + m_pan_y;

        for (int x = 0; x < w; ++x) {
            const float px = (static_cast<float>(x) - 160.0f) / 100.0f + m_pan_x;

            float u = 0.0f;
            float v = 0.0f;
            float fog = 1.0f;

            switch (m_deform_type) {
            // ----------------------------------------------------------------
            // Mode 0: Dual Symmetric Infinite Planes (Floor & Ceiling)
            // ----------------------------------------------------------------
            case DeformType::SymmetricPlanes: {
                const float abs_y = std::abs(py);
                if (abs_y < 0.04f) {
                    raw[y * w + x] = 0; // Horizon gap
                    continue;
                }
                u = (0.5f * px / abs_y) * 32.0f + m_scroll_u;
                v = (0.5f / abs_y) * 32.0f + m_scroll_v;
                fog = std::clamp(abs_y / 0.35f, 0.15f, 1.0f);
                break;
            }

            // ----------------------------------------------------------------
            // Mode 1: Logarithmic Swirl Tunnel
            // ----------------------------------------------------------------
            case DeformType::SwirlTunnel: {
                const float r = std::sqrt(px * px + py * py);
                if (r < 0.03f) {
                    raw[y * w + x] = 0;
                    continue;
                }
                const float a = std::atan2(py, px) + 0.15f * std::sin(r * 8.0f - m_time * 2.0f);
                const float s = 0.8f * std::cos(6.0f * a);
                u = (0.85f / (r + 0.2f * s)) * 28.0f + m_scroll_v;
                v = (3.0f * a / pi) * 20.0f + m_scroll_u;
                fog = std::clamp(r / 0.45f, 0.2f, 1.0f);
                break;
            }

            // ----------------------------------------------------------------
            // Mode 2: Concentric Harmonic Waves
            // ----------------------------------------------------------------
            case DeformType::ConcentricWaves: {
                const float r = std::sqrt(px * px + py * py);
                const float a = std::atan2(py, px);
                u = (a / pi) * 32.0f + m_scroll_u;
                v = (r + r * std::sin(r * 10.0f - m_time * 4.0f)) * 32.0f + m_scroll_v;
                fog = 1.0f;
                break;
            }

            // ----------------------------------------------------------------
            // Mode 3: TV CRT Barrel Warp & Vignette
            // ----------------------------------------------------------------
            case DeformType::TvBarrelWarp:
            default: {
                const float dx = px * px;
                const float dy = py * py;
                const float wx = px * (1.0f + dy * 0.32f);
                const float wy = py * (1.0f + dx * 0.42f);

                u = (wx * 0.5f + 0.5f) * 64.0f + m_scroll_u;
                v = (wy * 0.5f + 0.5f) * 64.0f + m_scroll_v;

                const float r2 = wx * wx + wy * wy;
                fog = (r2 < 1.4f) ? (1.0f - r2 * 0.55f) : 0.0f;
                break;
            }
            }

            const int iu = static_cast<int>(std::floor(u)) & 63;
            const int iv = static_cast<int>(std::floor(v)) & 63;

            const uint8_t base_c = tex[iv * tex_size + iu];
            const uint8_t final_c = static_cast<uint8_t>(std::clamp(static_cast<int>(static_cast<float>(base_c) * fog), 0, 255));
            raw[y * w + x] = final_c;
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
