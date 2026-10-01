#include "bump_effect.hh"

#include <algorithm>
#include <cmath>

namespace demoscene {

bump_effect::bump_effect()
    : m_heightmap(vga_canvas::pixel_count, 0)
    , m_lightmap(256 * 256, 0)
{
    generate_lightmap();
    generate_heightmap(0);
    on_enter();
}

void bump_effect::generate_lightmap() {
    constexpr int size = 256;
    constexpr float half_size = 128.0f;

    for (int y = 0; y < size; ++y) {
        const float ny = (static_cast<float>(y) - half_size) / half_size;
        const float ny2 = ny * ny;

        for (int x = 0; x < size; ++x) {
            const float nx = (static_cast<float>(x) - half_size) / half_size;
            const float r2 = nx * nx + ny2;

            uint8_t val = 0;
            if (r2 <= 1.0f) {
                const float nz = std::sqrt(1.0f - r2);
                // Diffuse + sharp specular highlight
                const float diffuse = nz;
                const float specular = std::pow(nz, 16.0f);
                const float total = std::clamp(0.15f * diffuse + 0.85f * specular, 0.0f, 1.0f);
                val = static_cast<uint8_t>(total * 254.0f + 1.0f);
            }
            m_lightmap[y * size + x] = val;
        }
    }
}

void bump_effect::generate_heightmap(int map_idx) {
    std::fill(m_heightmap.begin(), m_heightmap.end(), 0);

    constexpr int w = vga_canvas::width;
    constexpr int h = vga_canvas::height;

    switch (map_idx) {
    case 0: { // Demoscene Emblem: Concentric beveled rings & radial spokes
        for (int y = 0; y < h; ++y) {
            const float dy = static_cast<float>(y - 100);
            for (int x = 0; x < w; ++x) {
                const float dx = static_cast<float>(x - 160);
                const float r = std::sqrt(dx * dx + dy * dy);
                const float angle = std::atan2(dy, dx);

                float height = 0.0f;
                // Beveled rings
                if (r > 20.0f && r < 85.0f) {
                    height += 40.0f * std::sin(r * 0.25f);
                }
                // Star spokes
                const float spoke = std::cos(angle * 8.0f);
                if (r < 75.0f && spoke > 0.0f) {
                    height += spoke * 35.0f;
                }
                // Center boss
                if (r < 18.0f) {
                    height += (18.0f - r) * 3.0f;
                }

                m_heightmap[y * w + x] = static_cast<uint8_t>(std::clamp(height, 0.0f, 255.0f));
            }
        }
        break;
    }
    case 1: { // Cyber Circuit: Grid traces and rectangular IC chips
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                float height = 0.0f;
                // Circuit traces
                if ((x % 24 == 0) || (y % 24 == 0) || ((x + y) % 36 == 0)) {
                    height += 50.0f;
                }
                // IC Chip blocks
                if (x >= 60 && x <= 110 && y >= 50 && y <= 90) height = 90.0f;
                if (x >= 210 && x <= 260 && y >= 110 && y <= 150) height = 90.0f;
                if (x >= 130 && x <= 190 && y >= 80 && y <= 120) height = 110.0f;

                m_heightmap[y * w + x] = static_cast<uint8_t>(std::clamp(height, 0.0f, 255.0f));
            }
        }
        break;
    }
    case 2: { // Greek Maze: Raised beveled stone labyrinth
        for (int y = 0; y < h; ++y) {
            const int ty = y / 16;
            for (int x = 0; x < w; ++x) {
                const int tx = x / 16;
                const bool wall = ((tx * 7 + ty * 13) % 5 == 0) || (tx % 3 == 0 && ty % 2 == 0);
                const int bx = x % 16;
                const int by = y % 16;
                float height = 0.0f;
                if (wall) {
                    const int border_dist = std::min({bx, 15 - bx, by, 15 - by});
                    height = static_cast<float>(border_dist * 14);
                }
                m_heightmap[y * w + x] = static_cast<uint8_t>(std::clamp(height, 0.0f, 255.0f));
            }
        }
        break;
    }
    default:
        break;
    }
}

void bump_effect::init_palette() {
    m_canvas.set_rgb(0, 10, 8, 14);

    switch (m_color_theme) {
    case 0: { // Polished Copper & Gold: Deep mahogany -> Radiant copper -> Blinding gold highlight
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(60 + u * 175),
                    static_cast<uint8_t>(20 + u * 110),
                    static_cast<uint8_t>(10 + u * 40));
            } else {
                const float u = (t - 0.6f) / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    255,
                    static_cast<uint8_t>(130 + u * 125),
                    static_cast<uint8_t>(50 + u * 205));
            }
        }
        break;
    }
    case 1: { // Synthwave Violet & Cyan: Deep midnight -> Hot magenta -> Electric cyan specular
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(40 + u * 190),
                    static_cast<uint8_t>(u * 30),
                    static_cast<uint8_t>(70 + u * 150));
            } else {
                const float u = (t - 0.6f) / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(230 - u * 180),
                    static_cast<uint8_t>(30 + u * 225),
                    255);
            }
        }
        break;
    }
    case 2: { // Polished Silver & Steel: Deep gunmetal -> Pure silver -> Blinding specular white
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            const uint8_t v = static_cast<uint8_t>(30 + t * 225);
            m_canvas.set_rgb(static_cast<uint8_t>(i), v, v, v);
        }
        break;
    }
    default:
        break;
    }
}

void bump_effect::on_enter() {
    init_palette();
    m_canvas.clear(0);
    m_light_x = 160.0f;
    m_light_y = 100.0f;
    m_time = 0.0f;
    m_manual_control = false;
    m_map_idx = 0;
    m_color_theme = 0;
    generate_heightmap(0);
}

void bump_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    // Manual light position control
    bool moved = false;
    if (in.held(sdlpp::scancode::left))  { m_light_x -= 140.0f * dt_sec; moved = true; }
    if (in.held(sdlpp::scancode::right)) { m_light_x += 140.0f * dt_sec; moved = true; }
    if (in.held(sdlpp::scancode::up))    { m_light_y -= 140.0f * dt_sec; moved = true; }
    if (in.held(sdlpp::scancode::down))  { m_light_y += 140.0f * dt_sec; moved = true; }

    if (moved) {
        m_manual_control = true;
        m_light_x = std::clamp(m_light_x, 20.0f, 300.0f);
        m_light_y = std::clamp(m_light_y, 20.0f, 180.0f);
    } else if (!m_manual_control) {
        // Automatic Lissajous orbit of spotlight
        m_light_x = 160.0f + 85.0f * std::cos(m_time * 1.8f);
        m_light_y = 100.0f + 55.0f * std::sin(m_time * 2.6f);
    }

    // Cycle heightmap pattern
    if (in.pressed(sdlpp::scancode::space)) {
        m_map_idx = (m_map_idx + 1) % 3;
        generate_heightmap(m_map_idx);
    }

    // Cycle theme
    if (in.pressed(sdlpp::scancode::c)) {
        m_color_theme = (m_color_theme + 1) % 3;
        init_palette();
    }

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }
}

void bump_effect::render(const neutrino::rect& viewport) {
    constexpr int w = vga_canvas::width;
    constexpr int h = vga_canvas::height;

    uint8_t* raw = m_canvas.raw_pixels();
    const uint8_t* hmap = m_heightmap.data();
    const uint8_t* lmap = m_lightmap.data();

    const int ilx = static_cast<int>(m_light_x);
    const int ily = static_cast<int>(m_light_y);

    constexpr int bump_depth = 1;

    for (int y = 1; y < h - 1; ++y) {
        const int row = y * w;
        const int dy = ily - y + 128;

        for (int x = 1; x < w - 1; ++x) {
            const int idx = row + x;

            // Surface gradient: (H[x+1] - H[x-1], H[y+1] - H[y-1])
            const int nx = static_cast<int>(hmap[idx + 1]) - static_cast<int>(hmap[idx - 1]);
            const int ny = static_cast<int>(hmap[idx + w]) - static_cast<int>(hmap[idx - w]);

            // Lightmap coordinate lookup
            int lx = (ilx - x + 128) - nx * bump_depth;
            int ly = dy - ny * bump_depth;

            lx = std::clamp(lx, 0, 255);
            ly = std::clamp(ly, 0, 255);

            raw[idx] = lmap[ly * 256 + lx];
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
