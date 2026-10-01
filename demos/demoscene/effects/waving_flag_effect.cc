#include "waving_flag_effect.hh"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>

namespace demoscene {

waving_flag_effect::waving_flag_effect() {
    init_palette();
    build_flag();
    on_enter();
}

void waving_flag_effect::init_palette() {
    m_canvas.set_rgb(0, 0, 0, 0); // Black

    // 1..31: Dark navy background gradient
    for (int i = 1; i < 32; ++i) {
        const float t = static_cast<float>(i) / 31.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(t * 15.0f),
            static_cast<uint8_t>(t * 20.0f),
            static_cast<uint8_t>(30.0f + t * 45.0f));
    }

    // 32..63: Swedish Flag Blue (Deep Royal Blue to Sky Cyan)
    for (int i = 0; i < 32; ++i) {
        const float t = static_cast<float>(i) / 31.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(32 + i),
            static_cast<uint8_t>(t * 50.0f),
            static_cast<uint8_t>(50.0f + t * 140.0f),
            static_cast<uint8_t>(140.0f + t * 115.0f));
    }

    // 64..95: Swedish Flag Yellow / Gold
    for (int i = 0; i < 32; ++i) {
        const float t = static_cast<float>(i) / 31.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(64 + i),
            static_cast<uint8_t>(180.0f + t * 75.0f),
            static_cast<uint8_t>(140.0f + t * 115.0f),
            static_cast<uint8_t>(t * 60.0f));
    }

    // 96..127: Skull White / Silver
    for (int i = 0; i < 32; ++i) {
        const float t = static_cast<float>(i) / 31.0f;
        const uint8_t val = static_cast<uint8_t>(80.0f + t * 175.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(96 + i), val, val, val);
    }

    // 128..159: Pirate Charcoal / Black
    for (int i = 0; i < 32; ++i) {
        const float t = static_cast<float>(i) / 31.0f;
        const uint8_t val = static_cast<uint8_t>(15.0f + t * 45.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(128 + i), val, val, val);
    }

    // 160..191: Copper / Fiery Orange
    for (int i = 0; i < 32; ++i) {
        const float t = static_cast<float>(i) / 31.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(160 + i),
            static_cast<uint8_t>(160.0f + t * 95.0f),
            static_cast<uint8_t>(t * 140.0f),
            static_cast<uint8_t>(t * 40.0f));
    }

    // 192..255: Full Spectrum Rainbow Gradient
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        const float r = std::sin(t * 6.28f + 0.0f) * 127.0f + 128.0f;
        const float g = std::sin(t * 6.28f + 2.09f) * 127.0f + 128.0f;
        const float b = std::sin(t * 6.28f + 4.18f) * 127.0f + 128.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(192 + i),
            static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b));
    }
}

void waving_flag_effect::build_flag() {
    m_particles.resize(num_particles);

    for (int y = 0; y < flag_rows; ++y) {
        for (int x = 0; x < flag_cols; ++x) {
            const size_t idx = static_cast<size_t>(y * flag_cols + x);
            auto& p = m_particles[idx];

            p.base_x = static_cast<float>(x) * 2.8f;
            p.base_y = static_cast<float>(y) * 2.4f - 60.0f;

            // Theme 0: Swedish Flag (Blue field with yellow cross)
            const bool swedish_cross = (x >= 24 && x <= 33) || (y >= 20 && y <= 27);
            p.color[0] = swedish_cross ? 75 : 40;

            // Theme 1: Jolly Roger (Dark field with skull & bones)
            const int dx = x - 40;
            const int dy = y - 25;
            const bool skull = (dx * dx * 1.2f + dy * dy <= 110.0f) ||
                               (std::abs(dx) < 6 && dy >= 8 && dy <= 15) ||
                               (std::abs(std::abs(dx) - std::abs(dy)) <= 2 && (dx * dx + dy * dy) < 360.0f);
            p.color[1] = skull ? 115 : 135;

            // Theme 2: Demoscene Copper Checkerboard
            const bool check = (((x / 8) ^ (y / 8)) & 1) != 0;
            p.color[2] = check ? 175 : 110;

            // Theme 3: Holographic Pride Rainbow
            p.color[3] = static_cast<uint8_t>(192 + ((x + y * 2) % 64));
        }
    }
}

void waving_flag_effect::on_enter() {
    m_canvas.clear(0);
    m_wave_amp = 22.0f;
    m_wind_speed = 3.5f;
    m_time = 0.0f;
}

void waving_flag_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    if (in.pressed(sdlpp::scancode::space)) {
        m_flag_theme = (m_flag_theme + 1) % 4;
    }

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }

    if (in.held(sdlpp::scancode::up))   m_wind_speed = std::min(8.0f, m_wind_speed + 2.0f * dt_sec);
    if (in.held(sdlpp::scancode::down)) m_wind_speed = std::max(1.0f, m_wind_speed - 2.0f * dt_sec);

    if (in.held(sdlpp::scancode::equals) || in.held(sdlpp::scancode::kp_plus)) {
        m_wave_amp = std::min(45.0f, m_wave_amp + 15.0f * dt_sec);
    }
    if (in.held(sdlpp::scancode::minus) || in.held(sdlpp::scancode::kp_minus)) {
        m_wave_amp = std::max(5.0f, m_wave_amp - 15.0f * dt_sec);
    }
}

void waving_flag_effect::render(const neutrino::rect& viewport) {
    m_canvas.clear(0);
    uint8_t* raw = m_canvas.raw_pixels();

    // 1. Draw subtle background vignette
    for (int y = 0; y < vga_canvas::height; ++y) {
        const uint8_t bg = static_cast<uint8_t>(1 + (y * 20 / vga_canvas::height));
        std::memset(raw + y * vga_canvas::width, bg, vga_canvas::width);
    }

    // 2. Draw vertical flagpole on left (x = 36..39)
    constexpr int pole_x = 38;
    for (int y = 18; y < 192; ++y) {
        raw[y * vga_canvas::width + pole_x - 1] = 110; // Dark chrome
        raw[y * vga_canvas::width + pole_x]     = 125; // Bright chrome highlight
        raw[y * vga_canvas::width + pole_x + 1] = 100;
    }
    // Pole gold finial sphere at top
    for (int dy = -4; dy <= 4; ++dy) {
        for (int dx = -4; dx <= 4; ++dx) {
            if (dx * dx + dy * dy <= 16) {
                raw[(18 + dy) * vga_canvas::width + pole_x + dx] = 85; // Gold
            }
        }
    }

    constexpr float camera_z = 240.0f;
    constexpr float origin_x = 42.0f;
    constexpr float origin_y = 100.0f;

    // 3. Render Billowing Cloth Lattice
    for (const auto& p : m_particles) {
        const float u = p.base_x / (static_cast<float>(flag_cols) * 2.8f);
        const float v = (p.base_y + 60.0f) / 120.0f;

        // Wave amplitude scales with distance from pole: u^0.75 (clamped at pole)
        const float flutter = std::pow(u, 0.75f) * m_wave_amp;

        // Traveling compound wave physics
        const float wave1 = std::sin(u * 9.5f - m_time * m_wind_speed);
        const float wave2 = std::cos(v * 6.0f + u * 4.0f - m_time * 2.2f);
        const float z = flutter * (wave1 * 0.75f + wave2 * 0.25f);

        // Lateral contraction due to billowing
        const float x_offset = -flutter * 0.28f * (wave1 * wave1);
        const float y_offset = std::sin(u * 5.0f - m_time * 2.8f) * (u * 10.0f);

        const float world_x = origin_x + p.base_x + x_offset;
        const float world_y = origin_y + p.base_y + y_offset;
        const float world_z = z + camera_z;

        if (world_z < 10.0f) continue;

        const float inv_z = 240.0f / world_z;
        const int sx = static_cast<int>(world_x * inv_z);
        const int sy = static_cast<int>(world_y * inv_z);

        if (sx >= 0 && sx < vga_canvas::width && sy >= 0 && sy < vga_canvas::height) {
            // Specular wind crest highlight based on wave derivative
            const float crest_derivative = std::cos(u * 9.5f - m_time * m_wind_speed);
            const int highlight = static_cast<int>(crest_derivative * 8.0f);

            const uint8_t base_c = p.color[m_flag_theme];
            const uint8_t final_c = static_cast<uint8_t>(std::clamp(static_cast<int>(base_c) + highlight, 1, 255));

            raw[sy * vga_canvas::width + sx] = final_c;

            // Draw 2x2 particle for foreground nodes
            if (u > 0.4f && crest_derivative > 0.5f && sx + 1 < vga_canvas::width) {
                raw[sy * vga_canvas::width + sx + 1] = final_c;
            }
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
