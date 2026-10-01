#include "copper_effect.hh"

#include <algorithm>
#include <cmath>

namespace demoscene {

copper_effect::copper_effect() {
    // Define 10 distinct harmonic oscillating bars with varied frequencies and phase offsets
    m_bars = {
        {1.2f, 48.0f, 0.0f,  20, sdlpp::color{255, 140,  40, 255}}, // Copper orange
        {1.7f, 42.0f, 1.2f,  24, sdlpp::color{255,  60,  60, 255}}, // Crimson red
        {0.9f, 52.0f, 2.5f,  18, sdlpp::color{255, 210,  60, 255}}, // Gold
        {2.1f, 36.0f, 4.0f,  22, sdlpp::color{ 40, 180, 255, 255}}, // Cyan
        {1.4f, 45.0f, 5.1f,  20, sdlpp::color{220,  50, 240, 255}}, // Magenta
        {0.7f, 55.0f, 3.2f,  26, sdlpp::color{ 60, 255, 120, 255}}, // Lime green
        {1.9f, 38.0f, 0.8f,  16, sdlpp::color{255, 100, 180, 255}}, // Hot pink
        {1.1f, 46.0f, 2.1f,  22, sdlpp::color{100, 140, 255, 255}}, // Royal blue
        {2.4f, 32.0f, 4.7f,  18, sdlpp::color{255, 170,  50, 255}}, // Amber
        {0.8f, 50.0f, 1.5f,  24, sdlpp::color{180,  80, 255, 255}}, // Violet
    };

    on_enter();
}

void copper_effect::init_palette() {
    m_canvas.set_rgb(0, 5, 5, 15); // Deep twilight background

    switch (m_palette_theme) {
    case 0: {
        // Metallic Copper & Gold theme:
        // 1..60: Bronze to rich reddish-brown copper
        // 61..180: Bright fiery copper to brilliant amber gold
        // 181..255: Golden shine to blinding white specular highlight
        for (int i = 1; i <= 60; ++i) {
            const float t = static_cast<float>(i) / 60.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(30 + t * 90),
                static_cast<uint8_t>(10 + t * 30),
                static_cast<uint8_t>(5  + t * 15));
        }
        for (int i = 61; i <= 180; ++i) {
            const float t = static_cast<float>(i - 61) / 119.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(120 + t * 135),
                static_cast<uint8_t>(40  + t * 140),
                static_cast<uint8_t>(20  + t * 40));
        }
        for (int i = 181; i <= 255; ++i) {
            const float t = static_cast<float>(i - 181) / 74.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                255,
                static_cast<uint8_t>(180 + t * 75),
                static_cast<uint8_t>(60  + t * 195));
        }
        break;
    }
    case 1: {
        // Synthwave / Cyberpunk Neon theme:
        // 1..85: Electric Cyan to Ice Blue
        // 86..170: Neon Magenta to Hot Pink
        // 171..255: Royal Purple to Blinding White
        for (int i = 1; i <= 85; ++i) {
            const float t = static_cast<float>(i) / 85.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(t * 40),
                static_cast<uint8_t>(60 + t * 190),
                static_cast<uint8_t>(120 + t * 135));
        }
        for (int i = 86; i <= 170; ++i) {
            const float t = static_cast<float>(i - 86) / 84.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(180 + t * 75),
                static_cast<uint8_t>(t * 40),
                static_cast<uint8_t>(140 + t * 100));
        }
        for (int i = 171; i <= 255; ++i) {
            const float t = static_cast<float>(i - 171) / 84.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(120 + t * 135),
                static_cast<uint8_t>(80 + t * 175),
                255);
        }
        break;
    }
    case 2: {
        // Rainbow Spectrum theme:
        for (int i = 1; i <= 255; ++i) {
            const float hue = static_cast<float>(i - 1) / 254.0f * 6.0f;
            const float c = 1.0f;
            const float x = c * (1.0f - std::abs(std::fmod(hue, 2.0f) - 1.0f));
            float r = 0.0f, g = 0.0f, b = 0.0f;
            if (hue < 1.0f)      { r = c; g = x; }
            else if (hue < 2.0f) { r = x; g = c; }
            else if (hue < 3.0f) { g = c; b = x; }
            else if (hue < 4.0f) { g = x; b = c; }
            else if (hue < 5.0f) { r = x; b = c; }
            else                 { r = c; b = x; }

            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(r * 255.0f),
                static_cast<uint8_t>(g * 255.0f),
                static_cast<uint8_t>(b * 255.0f));
        }
        break;
    }
    default:
        break;
    }
}

void copper_effect::on_enter() {
    init_palette();
    m_time = 0.0f;
    m_speed = 1.0f;
    m_bar_count = 6;
    m_mirror_floor = true;
}

void copper_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec * m_speed;

    if (in.pressed(sdlpp::scancode::up))   m_bar_count = std::min(10, m_bar_count + 1);
    if (in.pressed(sdlpp::scancode::down)) m_bar_count = std::max(2, m_bar_count - 1);

    if (in.held(sdlpp::scancode::left))  m_speed = std::max(0.2f, m_speed - 0.8f * dt_sec);
    if (in.held(sdlpp::scancode::right)) m_speed = std::min(3.0f, m_speed + 0.8f * dt_sec);

    if (in.pressed(sdlpp::scancode::space)) {
        m_palette_theme = (m_palette_theme + 1) % 3;
        init_palette();
    }

    if (in.pressed(sdlpp::scancode::m)) {
        m_mirror_floor = !m_mirror_floor;
    }

    if (in.pressed(sdlpp::scancode::r)) {
        m_speed = 1.0f;
        m_bar_count = 6;
        m_palette_theme = 0;
        m_mirror_floor = true;
        init_palette();
    }
}

void copper_effect::render(const neutrino::rect& viewport) {
    // Clear canvas
    m_canvas.clear(0);

    constexpr int horizon_y = 135;
    constexpr float mid_y = 65.0f;

    // 1. Calculate per-scanline raster bar intensity accumulator
    std::array<float, horizon_y> scanline_intensity{};
    scanline_intensity.fill(0.0f);

    for (int i = 0; i < m_bar_count; ++i) {
        const auto& bar = m_bars[static_cast<size_t>(i)];

        // Compute vertical center position with compound sine wave
        const float bar_y = mid_y + bar.amp * std::sin(bar.freq * m_time + bar.phase)
                                  + (bar.amp * 0.25f) * std::cos(bar.freq * 0.6f * m_time);

        const int top_y = static_cast<int>(bar_y - static_cast<float>(bar.height) * 0.5f);
        const int bot_y = top_y + bar.height;

        for (int y = top_y; y < bot_y; ++y) {
            if (y >= 0 && y < horizon_y) {
                // Cylindrical specular lighting profile across the bar's thickness:
                // Peak brightness at 30% from the top (specular highlight crest)
                const float u = static_cast<float>(y - top_y) / static_cast<float>(bar.height);
                float profile = 0.0f;
                if (u < 0.35f) {
                    profile = std::sin((u / 0.35f) * 1.5708f); // Fast rise to specular peak
                } else {
                    profile = std::cos(((u - 0.35f) / 0.65f) * 1.5708f); // Soft gradient falloff
                }

                scanline_intensity[static_cast<size_t>(y)] += profile * 0.75f;
            }
        }
    }

    // 2. Render Copper Bars (upper half) with 3D horizontal cylindrical bulge
    for (int y = 0; y < horizon_y; ++y) {
        const float intensity = scanline_intensity[static_cast<size_t>(y)];
        if (intensity <= 0.001f) continue;

        for (int x = 0; x < vga_canvas::width; ++x) {
            // Horizontal curvature shading (darker at screen edges, brighter in center)
            const float nx = static_cast<float>(x - 160) / 160.0f;
            const float horiz_shade = 1.0f - 0.35f * (nx * nx);
            const float total = intensity * horiz_shade;

            const uint8_t color_idx = static_cast<uint8_t>(
                std::clamp(total * 254.0f + 1.0f, 1.0f, 255.0f)
            );
            m_canvas.put_pixel_fast(x, y, color_idx);
        }
    }

    // 3. Render Perspective Reflective Mirror Floor (lower half)
    if (m_mirror_floor) {
        for (int y = horizon_y; y < vga_canvas::height; ++y) {
            // Mirror source Y with perspective compression
            const float depth = static_cast<float>(y - horizon_y + 1);
            const int src_y = horizon_y - static_cast<int>(depth * 0.95f);

            float reflect_intensity = 0.0f;
            if (src_y >= 0 && src_y < horizon_y) {
                // Dim reflection with distance from the horizon
                const float damping = std::max(0.0f, 0.55f - (depth / static_cast<float>(vga_canvas::height - horizon_y)) * 0.35f);
                reflect_intensity = scanline_intensity[static_cast<size_t>(src_y)] * damping;
            }

            // Retro perspective grid lines on the floor
            const bool is_grid_h = ((y - horizon_y) % 8 == 0);

            for (int x = 0; x < vga_canvas::width; ++x) {
                const float nx = static_cast<float>(x - 160);
                const float perspective_x = nx / (depth * 0.08f + 0.5f);
                const bool is_grid_v = (std::abs(std::fmod(perspective_x, 30.0f)) < 1.0f);

                float final_intensity = reflect_intensity;
                if (is_grid_h || is_grid_v) {
                    final_intensity = std::min(1.0f, final_intensity + 0.15f);
                }

                if (final_intensity > 0.005f) {
                    const uint8_t color_idx = static_cast<uint8_t>(
                        std::clamp(final_intensity * 254.0f + 1.0f, 1.0f, 255.0f)
                    );
                    m_canvas.put_pixel_fast(x, y, color_idx);
                }
            }
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
