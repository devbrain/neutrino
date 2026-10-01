#include "linedance_effect.hh"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace demoscene {

linedance_effect::linedance_effect()
    : m_points(chain_nodes, euler::vec2<float>{160.0f, 100.0f})
{
    on_enter();
}

void linedance_effect::on_enter() {
    m_canvas.clear(0);
    init_palette();
    for (auto& p : m_points) {
        p = {160.0f, 100.0f};
    }
    m_damping = 0.85f;
    m_motion_blur = true;
    m_time = 0.0f;
}

void linedance_effect::init_palette() {
    m_canvas.set_rgb_6bit(0, 0, 0, 0); // Void black

    switch (m_theme) {
    case 0: { // Electric Neon Cyan / Blue
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(u * 40.0f),
                    static_cast<uint8_t>(40.0f + u * 180.0f),
                    static_cast<uint8_t>(80.0f + u * 175.0f));
            } else {
                const float u = (t - 0.6f) / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(40.0f + u * 215.0f),
                    255,
                    255);
            }
        }
        break;
    }
    case 1: { // Sunset Flare / Magma Orange
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(std::min(255.0f, t * 300.0f)),
                static_cast<uint8_t>(t * t * 210.0f),
                static_cast<uint8_t>(t * t * t * 80.0f));
        }
        break;
    }
    case 2: { // Aurora Emerald Phosphor
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(t * t * 80.0f),
                static_cast<uint8_t>(40.0f + t * 215.0f),
                static_cast<uint8_t>(60.0f + t * 160.0f));
        }
        break;
    }
    case 3: { // Amiga Copper Rainbow
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f * 6.0f;
            const int seg = static_cast<int>(t);
            const float f = t - static_cast<float>(seg);
            const uint8_t q = static_cast<uint8_t>(f * 255.0f);
            const uint8_t p = 255 - q;

            switch (seg % 6) {
            case 0: m_canvas.set_rgb(static_cast<uint8_t>(i), 255, q, 0); break;
            case 1: m_canvas.set_rgb(static_cast<uint8_t>(i), p, 255, 0); break;
            case 2: m_canvas.set_rgb(static_cast<uint8_t>(i), 0, 255, q); break;
            case 3: m_canvas.set_rgb(static_cast<uint8_t>(i), 0, p, 255); break;
            case 4: m_canvas.set_rgb(static_cast<uint8_t>(i), q, 0, 255); break;
            default:m_canvas.set_rgb(static_cast<uint8_t>(i), 255, 0, p); break;
            }
        }
        break;
    }
    }
}

void linedance_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec * 2.0f;

    if (in.pressed(sdlpp::scancode::space)) {
        m_mode = (m_mode + 1) % 3;
    }

    if (in.pressed(sdlpp::scancode::a)) {
        m_motion_blur = !m_motion_blur;
    }

    if (in.pressed(sdlpp::scancode::c)) {
        m_theme = (m_theme + 1) % 4;
        init_palette();
    }

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }

    if (in.held(sdlpp::scancode::equals) || in.held(sdlpp::scancode::kp_plus)) {
        m_damping = std::min(1.8f, m_damping + 0.5f * dt_sec);
    }
    if (in.held(sdlpp::scancode::minus) || in.held(sdlpp::scancode::kp_minus)) {
        m_damping = std::max(0.2f, m_damping - 0.5f * dt_sec);
    }

    // Lissajous multi-harmonic orbital head
    constexpr float pi = static_cast<float>(std::numbers::pi);
    const float aa = m_time / 1.37f;
    const float rx = std::abs(std::sin(std::sin(m_time / 4.1f) * pi) * 90.0f) + 12.0f;
    const float ry = std::abs(std::cos(std::cos(m_time / 1.3f) * pi) * 90.0f) + 12.0f;
    const float xx = std::cos(std::cos(m_time / 2.0f) * pi) * rx;
    const float yy = std::sin(std::cos(m_time / 2.7f) * pi) * ry;

    const float hx = 160.0f + xx * std::cos(aa) + yy * std::sin(aa);
    const float hy = 100.0f - xx * std::sin(aa) + yy * std::cos(aa);

    m_points[0] = {hx, hy};

    // Coupled spring relaxation
    const float k = std::sin(m_time / 15.0f) * 0.8f + m_damping;
    for (size_t i = 1; i < chain_nodes; ++i) {
        const float div = 2.0f + k / static_cast<float>(chain_nodes);
        m_points[i] = (m_points[i] + m_points[i - 1]) / div;
    }
}

void linedance_effect::render(const neutrino::rect& viewport) {
    uint8_t* raw = m_canvas.raw_pixels();

    // 1. CRT Phosphor Decay or Clear
    if (m_motion_blur) {
        for (int i = 0; i < vga_canvas::pixel_count; ++i) {
            raw[i] = static_cast<uint8_t>((static_cast<uint32_t>(raw[i]) * 220) >> 8);
        }
    } else {
        m_canvas.clear(0);
    }

    // Bresenham line drawing helper
    auto draw_line = [raw](int x0, int y0, int x1, int y1, uint8_t col) {
        int dx = std::abs(x1 - x0);
        int sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0);
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;

        while (true) {
            if (x0 >= 0 && x0 < vga_canvas::width && y0 >= 0 && y0 < vga_canvas::height) {
                const int idx = y0 * vga_canvas::width + x0;
                raw[idx] = std::max(raw[idx], col);
            }
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    };

    // 2. Render symmetric lines along the spring chain
    constexpr int w = vga_canvas::width;
    constexpr int h = vga_canvas::height;

    for (size_t i = 1; i < chain_nodes; ++i) {
        const int x1 = std::clamp(static_cast<int>(m_points[i].x()), 0, w - 1);
        const int y1 = std::clamp(static_cast<int>(m_points[i].y()), 0, h - 1);
        const int x2 = std::clamp(static_cast<int>(m_points[i - 1].x()), 0, w - 1);
        const int y2 = std::clamp(static_cast<int>(m_points[i - 1].y()), 0, h - 1);

        // Color graded along chain nodes: head is bright 255, tail fades to 60
        const uint8_t col = static_cast<uint8_t>(255 - (i * 190) / chain_nodes);

        if (m_mode == 0) { // 4-Way Kaleidoscope Mirror Symmetry
            draw_line(x1, y1, x2, y2, col);
            draw_line(x1, (h - 1) - y1, x2, (h - 1) - y2, col);
            draw_line((w - 1) - x1, y1, (w - 1) - x2, y2, col);
            draw_line((w - 1) - x1, (h - 1) - y1, (w - 1) - x2, (h - 1) - y2, col);
        } else if (m_mode == 1) { // 8-Way Octagonal Mandala
            draw_line(x1, y1, x2, y2, col);
            draw_line(x1, (h - 1) - y1, x2, (h - 1) - y2, col);
            draw_line((w - 1) - x1, y1, (w - 1) - x2, y2, col);
            draw_line((w - 1) - x1, (h - 1) - y1, (w - 1) - x2, (h - 1) - y2, col);

            // Diagonal transpose mapping
            const int tx1 = std::clamp(160 + (y1 - 100), 0, w - 1);
            const int ty1 = std::clamp(100 + (x1 - 160) * 100 / 160, 0, h - 1);
            const int tx2 = std::clamp(160 + (y2 - 100), 0, w - 1);
            const int ty2 = std::clamp(100 + (x2 - 160) * 100 / 160, 0, h - 1);

            draw_line(tx1, ty1, tx2, ty2, static_cast<uint8_t>(col * 3 / 4));
            draw_line((w - 1) - tx1, (h - 1) - ty1, (w - 1) - tx2, (h - 1) - ty2, static_cast<uint8_t>(col * 3 / 4));
        } else { // Dual Ribbon Cross-Wave
            draw_line(x1, y1, x2, y2, col);
            draw_line((w - 1) - x1, (h - 1) - y1, (w - 1) - x2, (h - 1) - y2, col);
            // Connecting cross struts every 8 nodes
            if (i % 8 == 0) {
                draw_line(x1, y1, (w - 1) - x1, (h - 1) - y1, static_cast<uint8_t>(col / 2));
            }
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
