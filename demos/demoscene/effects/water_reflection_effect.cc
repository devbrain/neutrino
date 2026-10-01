#include "water_reflection_effect.hh"

#include <algorithm>
#include <cmath>

namespace demoscene {

water_reflection_effect::water_reflection_effect()
    : m_skyline_buffer(vga_canvas::width * horizon_y, 0)
{
    generate_skyline();
    on_enter();
}

void water_reflection_effect::generate_skyline() {
    std::fill(m_skyline_buffer.begin(), m_skyline_buffer.end(), 0);
    constexpr int w = vga_canvas::width;
    constexpr int h = horizon_y;

    if (m_skyline_type == 0) { // Cyberpunk Metropolis
        // Sky gradient
        for (int y = 0; y < h; ++y) {
            const uint8_t sky_col = static_cast<uint8_t>(std::clamp((y * 25) / h + 5, 5, 30));
            for (int x = 0; x < w; ++x) {
                m_skyline_buffer[y * w + x] = sky_col;
            }
        }

        // Stars & Moon
        m_skyline_buffer[18 * w + 260] = 50; // Moon
        m_skyline_buffer[18 * w + 261] = 55;
        m_skyline_buffer[19 * w + 260] = 55;
        m_skyline_buffer[19 * w + 261] = 60;

        for (int i = 0; i < 40; ++i) {
            const int sx = (i * 79 + 17) % w;
            const int sy = (i * 43 + 7) % (h - 35);
            m_skyline_buffer[sy * w + sx] = 45;
        }

        // Skyscrapers
        struct building { int x, width, height; uint8_t col; };
        const std::array<building, 16> towers = {{
            {0, 25, 45, 35}, {20, 22, 60, 38}, {38, 28, 75, 40}, {62, 18, 50, 36},
            {76, 32, 85, 42}, {104, 24, 65, 39}, {124, 30, 92, 44}, {150, 22, 55, 37},
            {168, 35, 80, 41}, {198, 26, 68, 39}, {220, 32, 88, 43}, {248, 20, 58, 38},
            {264, 30, 72, 40}, {290, 24, 62, 38}, {308, 20, 48, 36}, {318, 15, 40, 35}
        }};

        for (const auto& b : towers) {
            const int top_y = h - b.height;
            for (int y = top_y; y < h; ++y) {
                for (int x = b.x; x < b.x + b.width && x < w; ++x) {
                    uint8_t pixel_col = b.col;
                    // Illuminated office windows
                    if (y > top_y + 4 && y < h - 4) {
                        if ((x % 4 == 1 || x % 4 == 2) && (y % 5 == 1 || y % 5 == 2)) {
                            if (((x * 13 + y * 7) % 5) != 0) {
                                pixel_col = 63; // Warm golden light
                            }
                        }
                    }
                    m_skyline_buffer[y * w + x] = pixel_col;
                }
            }
            // Antenna on top
            const int ant_x = b.x + b.width / 2;
            for (int y = std::max(0, top_y - 8); y < top_y; ++y) {
                if (ant_x < w) m_skyline_buffer[y * w + ant_x] = 55;
            }
            if (top_y >= 8 && ant_x < w) m_skyline_buffer[(top_y - 8) * w + ant_x] = 62; // Red beacon
        }
    } else if (m_skyline_type == 1) { // Alpine Sunset Peaks
        // Sunset sky gradient
        for (int y = 0; y < h; ++y) {
            const float t = static_cast<float>(y) / static_cast<float>(h);
            const uint8_t sky_col = static_cast<uint8_t>(10 + t * 45.0f);
            for (int x = 0; x < w; ++x) {
                m_skyline_buffer[y * w + x] = sky_col;
            }
        }

        // Sun disc at horizon
        for (int y = 35; y < 85; ++y) {
            for (int x = 135; x < 185; ++x) {
                const int dx = x - 160;
                const int dy = y - 60;
                if (dx * dx + dy * dy < 20 * 20) {
                    m_skyline_buffer[y * w + x] = 63;
                }
            }
        }

        // Mountain silhouettes
        for (int x = 0; x < w; ++x) {
            const float fx = static_cast<float>(x);
            const int m1 = 50 + static_cast<int>(std::sin(fx * 0.02f) * 20.0f + std::sin(fx * 0.05f) * 12.0f);
            const int m2 = 65 + static_cast<int>(std::sin(fx * 0.035f + 1.2f) * 16.0f + std::sin(fx * 0.08f) * 8.0f);

            for (int y = m1; y < h; ++y) m_skyline_buffer[y * w + x] = 32;
            for (int y = m2; y < h; ++y) m_skyline_buffer[y * w + x] = 22;
        }
    } else { // Cosmic Citadel
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                m_skyline_buffer[y * w + x] = static_cast<uint8_t>(5 + (y * 20) / h);
            }
        }
        // Floating monoliths
        for (int y = 20; y < 65; ++y) {
            for (int x = 140; x < 180; ++x) {
                m_skyline_buffer[y * w + x] = 45;
            }
        }
        // Spire
        for (int y = 5; y < 80; ++y) {
            m_skyline_buffer[y * w + 160] = 60;
            if (y > 40) {
                m_skyline_buffer[y * w + 159] = 55;
                m_skyline_buffer[y * w + 161] = 55;
            }
        }
    }
}

void water_reflection_effect::on_enter() {
    m_canvas.clear(0);
    init_palette();
    m_wave_amp = 1.0f;
    m_wind_speed = 1.0f;
    m_time = 0.0f;
}

void water_reflection_effect::init_palette() {
    m_canvas.set_rgb_6bit(0, 0, 0, 0);

    // 0..127: Skyline colors
    if (m_theme == 0) { // Twilight Indigo
        for (int i = 1; i < 128; ++i) {
            const float t = static_cast<float>(i) / 127.0f;
            if (t < 0.4f) {
                const float u = t / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(u * 30.0f),
                    static_cast<uint8_t>(10.0f + u * 40.0f),
                    static_cast<uint8_t>(30.0f + u * 90.0f));
            } else if (t < 0.8f) {
                const float u = (t - 0.4f) / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(30.0f + u * 180.0f),
                    static_cast<uint8_t>(50.0f + u * 60.0f),
                    static_cast<uint8_t>(120.0f + u * 40.0f));
            } else {
                const float u = (t - 0.8f) / 0.2f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(210.0f + u * 45.0f),
                    static_cast<uint8_t>(110.0f + u * 135.0f),
                    static_cast<uint8_t>(160.0f + u * 95.0f));
            }
        }
    } else if (m_theme == 1) { // Golden Amber Sunset
        for (int i = 1; i < 128; ++i) {
            const float t = static_cast<float>(i) / 127.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(std::min(255.0f, t * 290.0f)),
                static_cast<uint8_t>(t * t * 210.0f),
                static_cast<uint8_t>(t * t * t * 70.0f));
        }
    } else { // Emerald Aurora
        for (int i = 1; i < 128; ++i) {
            const float t = static_cast<float>(i) / 127.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(t * t * 60.0f),
                static_cast<uint8_t>(40.0f + t * 215.0f),
                static_cast<uint8_t>(60.0f + t * 140.0f));
        }
    }

    // 128..255: Tinted Water Surface Reflection bank (reflet.c formula: RGB / 2 + water cyan tint)
    for (int k = 0; k < 128; ++k) {
        const auto& col = m_canvas.get_color(static_cast<uint8_t>(k));
        const uint8_t wr = col.r / 3;
        const uint8_t wg = col.g / 2 + 10;
        const uint8_t wb = col.b / 2 + 35;
        m_canvas.set_rgb(static_cast<uint8_t>(k + 128), wr, wg, wb);
    }
}

void water_reflection_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    if (in.pressed(sdlpp::scancode::space)) {
        m_skyline_type = (m_skyline_type + 1) % 3;
        generate_skyline();
    }

    if (in.pressed(sdlpp::scancode::c)) {
        m_theme = (m_theme + 1) % 3;
        init_palette();
    }

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }

    if (in.held(sdlpp::scancode::up))   m_wave_amp = std::min(2.5f, m_wave_amp + 1.2f * dt_sec);
    if (in.held(sdlpp::scancode::down)) m_wave_amp = std::max(0.1f, m_wave_amp - 1.2f * dt_sec);

    if (in.held(sdlpp::scancode::right)) m_wind_speed = std::min(3.0f, m_wind_speed + 1.2f * dt_sec);
    if (in.held(sdlpp::scancode::left))  m_wind_speed = std::max(0.2f, m_wind_speed - 1.2f * dt_sec);
}

void water_reflection_effect::render(const neutrino::rect& viewport) {
    uint8_t* raw = m_canvas.raw_pixels();
    constexpr int w = vga_canvas::width;
    constexpr int h = vga_canvas::height;
    constexpr int water_h = h - horizon_y;

    // 1. Copy skyline onto top half
    std::copy_n(m_skyline_buffer.data(), w * horizon_y, raw);

    // 2. Render rippling water mirror reflection (reflet.c compound waves)
    const float vt = m_time * 2.8f * m_wind_speed;
    const float ht = m_time * 3.5f * m_wind_speed;
    const float amp = m_wave_amp;

    for (int j = 0; j < water_h; ++j) {
        const float fj = static_cast<float>(j);

        // Vertical wave movement with distance perspective factor (j / 20.0f)
        const int sample_y = std::clamp(
            horizon_y - 1 - j + static_cast<int>(std::sin(fj / 3.0f + vt) * (fj / 18.0f) * amp),
            0, horizon_y - 1
        );

        // Horizontal ripple oscillation
        const int offset_x = static_cast<int>(std::cos(fj / 4.0f + ht) * (fj / 22.0f) * amp);

        const uint8_t* src_row = m_skyline_buffer.data() + sample_y * w;
        uint8_t* dst_row = raw + (horizon_y + j) * w;

        for (int x = 0; x < w; ++x) {
            const int src_x = std::clamp(x + offset_x, 0, w - 1);
            // reflet.c palette mapping: bit 7 (128) maps into tinted watery reflection palette bank
            dst_row[x] = static_cast<uint8_t>(src_row[src_x] | 0x80);
        }
    }

    // Waterline demarcation / specular highlight
    for (int x = 0; x < w; ++x) {
        raw[horizon_y * w + x] = 220;
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
