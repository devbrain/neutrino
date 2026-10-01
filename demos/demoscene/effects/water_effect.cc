#include "water_effect.hh"

#include <algorithm>
#include <cmath>
#include <random>

namespace demoscene {

water_effect::water_effect()
    : m_height1(vga_canvas::pixel_count, 0.0f)
    , m_height2(vga_canvas::pixel_count, 0.0f)
    , m_background(vga_canvas::pixel_count, 0)
{
    generate_background();
    on_enter();
}

void water_effect::generate_background() {
    // Generate procedural pool tile floor mosaic
    for (int y = 0; y < vga_canvas::height; ++y) {
        for (int x = 0; x < vga_canvas::width; ++x) {
            const int tx = x / 16;
            const int ty = y / 16;
            const bool border = (x % 16 == 0) || (y % 16 == 0);

            // Grout line
            if (border) {
                m_background[y * vga_canvas::width + x] = 10;
                continue;
            }

            // Tile shade with subtle radial medallion
            const int dx = x - 160;
            const int dy = y - 100;
            const float r = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            const int tile_var = ((tx ^ ty) & 1) ? 20 : 35;
            const int pattern = static_cast<int>(std::sin(r * 0.12f) * 12.0f);

            m_background[y * vga_canvas::width + x] = static_cast<uint8_t>(std::clamp(tile_var + pattern + 40, 20, 180));
        }
    }
}

void water_effect::init_palette() {
    m_canvas.set_rgb(0, 4, 8, 16);

    switch (m_theme) {
    case 0: { // Caribbean Azure Pool: Deep indigo -> Vibrant cyan -> Sparkling white
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.65f) {
                const float u = t / 0.65f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(u * 40),
                    static_cast<uint8_t>(30 + u * 170),
                    static_cast<uint8_t>(70 + u * 185));
            } else {
                const float u = (t - 0.65f) / 0.35f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(40 + u * 215),
                    static_cast<uint8_t>(200 + u * 55),
                    255);
            }
        }
        break;
    }
    case 1: { // Deep Emerald Lagoon: Dark moss -> Jade turquoise -> Sunlit gold
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.65f) {
                const float u = t / 0.65f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(u * 50),
                    static_cast<uint8_t>(40 + u * 180),
                    static_cast<uint8_t>(30 + u * 90));
            } else {
                const float u = (t - 0.65f) / 0.35f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(50 + u * 205),
                    static_cast<uint8_t>(220 + u * 35),
                    static_cast<uint8_t>(120 + u * 135));
            }
        }
        break;
    }
    case 2: { // Molten Magma Pool: Obsidian black -> Crimson lava -> Blinding yellow heat
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.55f) {
                const float u = t / 0.55f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(60 + u * 195),
                    static_cast<uint8_t>(u * 50),
                    static_cast<uint8_t>(u * 15));
            } else {
                const float u = (t - 0.55f) / 0.45f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    255,
                    static_cast<uint8_t>(50 + u * 205),
                    static_cast<uint8_t>(15 + u * 180));
            }
        }
        break;
    }
    default:
        break;
    }
}

void water_effect::drop_splash(int cx, int cy, float depth, int radius) {
    for (int dy = -radius; dy <= radius; ++dy) {
        const int py = cy + dy;
        if (py < 1 || py >= vga_canvas::height - 1) continue;
        for (int dx = -radius; dx <= radius; ++dx) {
            const int px = cx + dx;
            if (px < 1 || px >= vga_canvas::width - 1) continue;

            const float dist2 = static_cast<float>(dx * dx + dy * dy);
            const float r2 = static_cast<float>(radius * radius);
            if (dist2 <= r2) {
                const float falloff = std::cos(std::sqrt(dist2) / static_cast<float>(radius) * 1.570796f);
                m_height1[py * vga_canvas::width + px] -= depth * falloff;
            }
        }
    }
}

void water_effect::on_enter() {
    init_palette();
    std::fill(m_height1.begin(), m_height1.end(), 0.0f);
    std::fill(m_height2.begin(), m_height2.end(), 0.0f);
    m_damping = 0.985f;
    m_refraction = 16.0f;
    m_ripple_x = 160.0f;
    m_ripple_y = 100.0f;
    m_time = 0.0f;
    m_theme = 0;

    // Initial splash
    drop_splash(160, 100, 350.0f, 6);
}

void water_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    // Auto-rain droplets
    static float rain_timer = 0.0f;
    rain_timer += dt_sec;
    if (rain_timer >= 0.25f) {
        rain_timer = 0.0f;
        const int rx = 20 + (rand() % (vga_canvas::width - 40));
        const int ry = 20 + (rand() % (vga_canvas::height - 40));
        drop_splash(rx, ry, 280.0f, 4);
    }

    // Interactive ripple controls
    bool moved = false;
    if (in.held(sdlpp::scancode::left))  { m_ripple_x = std::max(10.0f, m_ripple_x - 120.0f * dt_sec); moved = true; }
    if (in.held(sdlpp::scancode::right)) { m_ripple_x = std::min(310.0f, m_ripple_x + 120.0f * dt_sec); moved = true; }
    if (in.held(sdlpp::scancode::up))    { m_ripple_y = std::max(10.0f, m_ripple_y - 120.0f * dt_sec); moved = true; }
    if (in.held(sdlpp::scancode::down))  { m_ripple_y = std::min(190.0f, m_ripple_y + 120.0f * dt_sec); moved = true; }

    if (moved) {
        drop_splash(static_cast<int>(m_ripple_x), static_cast<int>(m_ripple_y), 180.0f, 3);
    }

    if (in.pressed(sdlpp::scancode::space)) {
        drop_splash(static_cast<int>(m_ripple_x), static_cast<int>(m_ripple_y), 450.0f, 7);
    }

    // Theme cycle
    if (in.pressed(sdlpp::scancode::c)) {
        m_theme = (m_theme + 1) % 3;
        init_palette();
    }

    // Viscosity
    if (in.held(sdlpp::scancode::w)) m_damping = std::min(0.995f, m_damping + 0.01f * dt_sec);
    if (in.held(sdlpp::scancode::s)) m_damping = std::max(0.920f, m_damping - 0.01f * dt_sec);

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }
}

void water_effect::render(const neutrino::rect& viewport) {
    constexpr int w = vga_canvas::width;
    constexpr int h = vga_canvas::height;

    // 1. 2D Wave equation simulation pass:
    // h2 = ((h1[x-1] + h1[x+1] + h1[y-1] + h1[y+1]) * 0.5 - h2) * damping
    float* h1 = m_height1.data();
    float* h2 = m_height2.data();

    for (int y = 1; y < h - 1; ++y) {
        const int row = y * w;
        for (int x = 1; x < w - 1; ++x) {
            const int idx = row + x;
            h2[idx] = ((h1[idx - 1] + h1[idx + 1] + h1[idx - w] + h1[idx + w]) * 0.5f - h2[idx]) * m_damping;
        }
    }

    // 2. Optical refraction & caustic rendering pass:
    uint8_t* raw = m_canvas.raw_pixels();
    const uint8_t* bg = m_background.data();

    for (int y = 1; y < h - 1; ++y) {
        const int row = y * w;
        for (int x = 1; x < w - 1; ++x) {
            const int idx = row + x;

            // Surface gradient normal components
            const float nx = h2[idx + 1] - h2[idx - 1];
            const float ny = h2[idx + w] - h2[idx - w];

            // Snell's law refractive displacement
            int rx = x - static_cast<int>(nx * m_refraction);
            int ry = y - static_cast<int>(ny * m_refraction);

            rx = std::clamp(rx, 0, w - 1);
            ry = std::clamp(ry, 0, h - 1);

            uint8_t col = bg[ry * w + rx];

            // Specular caustic crest glint
            const float crest = (nx * nx + ny * ny);
            if (crest > 0.08f) {
                col = static_cast<uint8_t>(std::min(255, col + static_cast<int>(crest * 90.0f)));
            }

            raw[idx] = col;
        }
    }

    // 3. Swap height buffers
    std::swap(m_height1, m_height2);

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
