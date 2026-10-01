#include "mesh_distort_effect.hh"
#include <onyx_font/bios_font.hh>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>

namespace demoscene {

mesh_distort_effect::mesh_distort_effect() {
    init_palette();
    on_enter();
}

void mesh_distort_effect::init_palette() {
    m_canvas.set_rgb(0, 0, 0, 0); // Black

    // 1..63: Copper / Amber Metallic Ramp
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        const uint8_t r = static_cast<uint8_t>(std::min(255.0f, t * 290.0f));
        const uint8_t g = static_cast<uint8_t>(t * 190.0f);
        const uint8_t b = static_cast<uint8_t>(t * t * 80.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
    }

    // 64..127: Cyber Neon Cyan & Blue Ramp
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        const uint8_t r = static_cast<uint8_t>(t * t * 60.0f);
        const uint8_t g = static_cast<uint8_t>(40.0f + t * 215.0f);
        const uint8_t b = static_cast<uint8_t>(100.0f + t * 155.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(64 + i), r, g, b);
    }

    // 128..191: Magenta / Violet Laser Ramp
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        const uint8_t r = static_cast<uint8_t>(80.0f + t * 175.0f);
        const uint8_t g = static_cast<uint8_t>(t * t * 80.0f);
        const uint8_t b = static_cast<uint8_t>(std::min(255.0f, 120.0f + t * 135.0f));
        m_canvas.set_rgb(static_cast<uint8_t>(128 + i), r, g, b);
    }

    // 192..255: Chrome & Diamond White Ramp
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        const uint8_t val = static_cast<uint8_t>(t * 255.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(192 + i), val, val, val);
    }
}

void mesh_distort_effect::on_enter() {
    m_canvas.clear(0);
    m_amplitude = 18.0f;
    m_frequency = 1.0f;
    m_time = 0.0f;
}

void mesh_distort_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    if (in.pressed(sdlpp::scancode::space)) {
        m_distort_mode = static_cast<DistortMode>((static_cast<int>(m_distort_mode) + 1) % static_cast<int>(DistortMode::Count));
    }

    if (in.pressed(sdlpp::scancode::m)) {
        m_scene_id = (m_scene_id + 1) % 3;
    }

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }

    if (in.held(sdlpp::scancode::equals) || in.held(sdlpp::scancode::kp_plus)) {
        m_amplitude = std::min(40.0f, m_amplitude + 15.0f * dt_sec);
    }
    if (in.held(sdlpp::scancode::minus) || in.held(sdlpp::scancode::kp_minus)) {
        m_amplitude = std::max(2.0f, m_amplitude - 15.0f * dt_sec);
    }

    if (in.held(sdlpp::scancode::up))   m_frequency = std::min(2.5f, m_frequency + 0.8f * dt_sec);
    if (in.held(sdlpp::scancode::down)) m_frequency = std::max(0.3f, m_frequency - 0.8f * dt_sec);
}

void mesh_distort_effect::render_source_scene(int scene_id, uint8_t* dst) {
    constexpr int w = vga_canvas::width;
    constexpr int h = vga_canvas::height;

    switch (scene_id) {
    // ------------------------------------------------------------------------
    // Scene 0: Classic Copper Logo with Drop Shadow
    // ------------------------------------------------------------------------
    case 0: {
        for (int y = 0; y < h; ++y) {
            const float fy = static_cast<float>(y);
            const float copper = std::sin(fy * 0.06f) * 0.5f + 0.5f;
            const uint8_t bg = static_cast<uint8_t>(copper * 25.0f);
            std::memset(dst + y * w, bg, w);
        }

        const auto& font = onyx_font::bios_font_8x8();
        constexpr std::string_view line1 = "NEUTRINO";
        constexpr std::string_view line2 = "DEMOSCENE";
        constexpr int scale = 3;

        auto draw_text = [&font, dst, w, h](std::string_view text, int cx, int cy, uint8_t color) {
            for (size_t ci = 0; ci < text.size(); ++ci) {
                const auto glyph = font.get_glyph(static_cast<uint8_t>(text[ci]));
                const int px = cx + static_cast<int>(ci) * 8 * scale;
                for (int gy = 0; gy < 8; ++gy) {
                    for (int gx = 0; gx < 8; ++gx) {
                        if (glyph.pixel(static_cast<uint16_t>(gx), static_cast<uint16_t>(gy))) {
                            for (int sy = 0; sy < scale; ++sy) {
                                for (int sx = 0; sx < scale; ++sx) {
                                    const int x = px + gx * scale + sx;
                                    const int y = cy + gy * scale + sy;
                                    if (x >= 0 && x < w && y >= 0 && y < h) {
                                        dst[y * w + x] = color;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        };

        // Drop shadow + embossed foreground
        draw_text(line1, (w - static_cast<int>(line1.size()) * 8 * scale) / 2 + 4, 64 + 4, 0);
        draw_text(line1, (w - static_cast<int>(line1.size()) * 8 * scale) / 2, 64, 55);

        draw_text(line2, (w - static_cast<int>(line2.size()) * 8 * scale) / 2 + 4, 104 + 4, 0);
        draw_text(line2, (w - static_cast<int>(line2.size()) * 8 * scale) / 2, 104, 115);
        break;
    }

    // ------------------------------------------------------------------------
    // Scene 1: Perspective Checkered Horizon
    // ------------------------------------------------------------------------
    case 1: {
        constexpr int horizon_y = 95;
        for (int y = 0; y < horizon_y; ++y) {
            const float t = static_cast<float>(y) / static_cast<float>(horizon_y);
            const uint8_t sky_col = static_cast<uint8_t>(64 + static_cast<int>(t * 40.0f));
            std::memset(dst + y * w, sky_col, w);
        }

        for (int y = horizon_y; y < h; ++y) {
            const float dy = static_cast<float>(y - horizon_y + 1);
            const float z = 140.0f / dy;
            for (int x = 0; x < w; ++x) {
                const float dx = static_cast<float>(x - 160);
                const float u = dx * z * 0.08f;
                const float v = z * 0.25f;
                const bool check = ((static_cast<int>(std::floor(u)) ^ static_cast<int>(std::floor(v))) & 1) != 0;
                dst[y * w + x] = check ? 175 : 135;
            }
        }
        break;
    }

    // ------------------------------------------------------------------------
    // Scene 2: Cyber Circuit Grid
    // ------------------------------------------------------------------------
    case 2:
    default: {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const bool grid = (x % 20 == 0 || y % 20 == 0);
                const bool node = (x % 60 == 0 && y % 60 == 0);
                uint8_t col = 70;
                if (grid) col = 105;
                if (node) col = 250;
                dst[y * w + x] = col;
            }
        }
        break;
    }
    }
}

void mesh_distort_effect::render(const neutrino::rect& viewport) {
    // 1. Render pristine source scene to buffer
    render_source_scene(m_scene_id, m_source_buffer.data());

    uint8_t* raw = m_canvas.raw_pixels();
    constexpr int w = vga_canvas::width;
    constexpr int h = vga_canvas::height;

    // 2. Apply 2D non-linear displacement distortion
    for (int y = 0; y < h; ++y) {
        const float fy = static_cast<float>(y);
        for (int x = 0; x < w; ++x) {
            const float fx = static_cast<float>(x);
            float dx = 0.0f;
            float dy = 0.0f;

            switch (m_distort_mode) {
            case DistortMode::LiquidRubber: {
                // Compound harmonic field from distort.cpp
                dx = m_amplitude * (std::sin(fx * 0.045f * m_frequency + m_time * 2.5f) +
                                    std::cos((fx + fy) * 0.028f - m_time * 1.8f));
                dy = m_amplitude * (std::cos(fy * 0.055f * m_frequency - m_time * 2.0f) +
                                    std::sin((fy - fx) * 0.024f + m_time * 1.5f));
                break;
            }

            case DistortMode::UnderwaterFlag: {
                dx = m_amplitude * std::sin(fy * 0.075f * m_frequency + m_time * 3.5f);
                dy = (m_amplitude * 0.5f) * std::cos(fx * 0.055f * m_frequency + m_time * 2.2f);
                break;
            }

            case DistortMode::PolarVortex: {
                const float cx = 160.0f + 35.0f * std::sin(m_time * 1.6f);
                const float cy = 100.0f + 25.0f * std::cos(m_time * 1.9f);
                const float px = fx - cx;
                const float py = fy - cy;
                const float r = std::sqrt(px * px + py * py);
                const float theta = std::atan2(py, px) + (m_amplitude * 0.075f) * std::sin(r * 0.045f * m_frequency - m_time * 3.0f);
                dx = (cx + std::cos(theta) * r) - fx;
                dy = (cy + std::sin(theta) * r) - fy;
                break;
            }

            case DistortMode::CrtWobble: {
                const float scanline_jitter = std::sin(fy * 0.35f * m_frequency + m_time * 14.0f) * (m_amplitude * 0.3f);
                const float barrel = ((fy - 100.0f) * (fy - 100.0f)) * 0.0006f * m_amplitude;
                dx = scanline_jitter + std::sin(m_time * 4.5f) * barrel;
                dy = std::cos(fx * 0.08f * m_frequency + m_time * 3.5f) * (m_amplitude * 0.25f);
                break;
            }

            default:
                break;
            }

            const int sx = std::clamp(static_cast<int>(fx + dx), 0, w - 1);
            const int sy = std::clamp(static_cast<int>(fy + dy), 0, h - 1);
            raw[y * w + x] = m_source_buffer[static_cast<size_t>(sy * w + sx)];
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
