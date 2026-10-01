#include "metaballs_effect.hh"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace demoscene {

metaballs_effect::metaballs_effect() {
    on_enter();
}

void metaballs_effect::init_palette() {
    m_canvas.set_rgb(0, 6, 8, 16); // Deep space background

    switch (m_color_theme) {
    case 0: { // Liquid Chrome: Steel blue -> Shimmering silver -> Blinding specular highlight
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.65f) {
                const float u = t / 0.65f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(20 + u * 160),
                    static_cast<uint8_t>(30 + u * 175),
                    static_cast<uint8_t>(50 + u * 205));
            } else {
                const float u = (t - 0.65f) / 0.35f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(180 + u * 75),
                    static_cast<uint8_t>(205 + u * 50),
                    255);
            }
        }
        break;
    }
    case 1: { // Synthwave Magenta: Deep violet -> Electric neon pink -> Ice white
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(60 + u * 180),
                    static_cast<uint8_t>(u * 30),
                    static_cast<uint8_t>(90 + u * 140));
            } else {
                const float u = (t - 0.6f) / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    255,
                    static_cast<uint8_t>(30 + u * 225),
                    static_cast<uint8_t>(230 + u * 25));
            }
        }
        break;
    }
    case 2: { // Emerald Acid / Toxic Slime: Dark moss -> Radiant lime green -> Toxic yellow
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(u * 40),
                    static_cast<uint8_t>(40 + u * 190),
                    static_cast<uint8_t>(20 + u * 50));
            } else {
                const float u = (t - 0.6f) / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(40 + u * 215),
                    255,
                    static_cast<uint8_t>(70 + u * 185));
            }
        }
        break;
    }
    default:
        break;
    }
}

void metaballs_effect::reset_balls() {
    m_balls.clear();
    // 5 dynamic metaballs with varied radii and orbital frequencies
    m_balls.push_back({{160.0f, 100.0f}, { 65.0f,  45.0f}, 36.0f});
    m_balls.push_back({{160.0f, 100.0f}, {-50.0f,  55.0f}, 32.0f});
    m_balls.push_back({{160.0f, 100.0f}, { 75.0f, -35.0f}, 28.0f});
    m_balls.push_back({{160.0f, 100.0f}, {-60.0f, -40.0f}, 34.0f});
    m_balls.push_back({{160.0f, 100.0f}, { 40.0f,  60.0f}, 26.0f});
}

void metaballs_effect::on_enter() {
    init_palette();
    reset_balls();
    m_canvas.clear(0);
    m_time = 0.0f;
    m_speed = 1.0f;
    m_threshold = 1.0f;
    m_mode = 0;
    m_color_theme = 0;
}

void metaballs_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec * m_speed;

    // Harmonically orbit the 5 metaballs in organic paths
    const float t = m_time;
    m_balls[0].pos = euler::vec2<float>{160.0f + 70.0f * std::sin(t * 1.3f), 100.0f + 45.0f * std::cos(t * 1.7f)};
    m_balls[1].pos = euler::vec2<float>{160.0f + 80.0f * std::cos(t * 0.9f), 100.0f + 40.0f * std::sin(t * 1.4f)};
    m_balls[2].pos = euler::vec2<float>{160.0f + 65.0f * std::sin(t * 1.8f + 1.2f), 100.0f + 50.0f * std::cos(t * 1.1f + 0.8f)};
    m_balls[3].pos = euler::vec2<float>{160.0f + 85.0f * std::cos(t * 1.5f - 0.5f), 100.0f + 35.0f * std::sin(t * 2.1f + 1.4f)};
    m_balls[4].pos = euler::vec2<float>{160.0f + 55.0f * std::sin(t * 2.3f - 1.0f), 100.0f + 55.0f * std::cos(t * 0.8f - 0.4f)};

    if (in.pressed(sdlpp::scancode::space)) {
        m_mode = (m_mode + 1) % 3;
    }

    if (in.pressed(sdlpp::scancode::c)) {
        m_color_theme = (m_color_theme + 1) % 3;
        init_palette();
    }

    if (in.held(sdlpp::scancode::up))   m_threshold = std::min(2.5f, m_threshold + 0.8f * dt_sec);
    if (in.held(sdlpp::scancode::down)) m_threshold = std::max(0.4f, m_threshold - 0.8f * dt_sec);

    if (in.held(sdlpp::scancode::w)) m_speed = std::min(3.0f, m_speed + 1.0f * dt_sec);
    if (in.held(sdlpp::scancode::s)) m_speed = std::max(0.2f, m_speed - 1.0f * dt_sec);

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }
}

void metaballs_effect::render(const neutrino::rect& viewport) {
    m_canvas.clear(0);

    uint8_t* raw = m_canvas.raw_pixels();

    // Cache ball positions and radius squared
    const size_t num_balls = m_balls.size();
    float bx[8], by[8], r2[8];
    for (size_t i = 0; i < num_balls; ++i) {
        bx[i] = m_balls[i].pos.x();
        by[i] = m_balls[i].pos.y();
        r2[i] = (m_balls[i].radius * m_balls[i].radius) * m_threshold;
    }

    constexpr float eps = 16.0f;

    for (int y = 0; y < vga_canvas::height; y += 2) {
        const float fy = static_cast<float>(y);
        float dy_sq[8];
        for (size_t i = 0; i < num_balls; ++i) {
            const float dy = fy - by[i];
            dy_sq[i] = dy * dy + eps;
        }

        uint8_t* row0 = raw + y * vga_canvas::width;
        uint8_t* row1 = row0 + vga_canvas::width;

        for (int x = 0; x < vga_canvas::width; x += 2) {
            const float fx = static_cast<float>(x);

            // Potential summation: V(x, y) = Σ R_i² / (dx² + dy² + ε)
            float potential = 0.0f;
            for (size_t i = 0; i < num_balls; ++i) {
                const float dx = fx - bx[i];
                potential += r2[i] / (dx * dx + dy_sq[i]);
            }

            uint8_t col = 0;

            switch (m_mode) {
            case 0: {
                // Continuous Iridescent Isosurface
                if (potential > 0.35f) {
                    const int val = static_cast<int>((potential - 0.35f) * 110.0f);
                    col = static_cast<uint8_t>(std::clamp(val + 1, 1, 255));
                }
                break;
            }
            case 1: {
                // Solid Organic Blob with Specular Rim
                if (potential >= 1.0f) {
                    const float rim = std::abs(potential - 1.0f);
                    if (rim < 0.12f) {
                        col = 255; // Specular edge rim
                    } else {
                        const int interior = static_cast<int>(140.0f + std::min(115.0f, (potential - 1.0f) * 40.0f));
                        col = static_cast<uint8_t>(interior);
                    }
                }
                break;
            }
            case 2: {
                // Equipotential Contour Rings
                const float ring = std::sin(potential * 7.0f);
                if (potential > 0.4f && ring > 0.4f) {
                    const int val = static_cast<int>(ring * 200.0f + 40.0f);
                    col = static_cast<uint8_t>(std::clamp(val, 1, 255));
                }
                break;
            }
            default:
                break;
            }

            const uint16_t double_pixel = static_cast<uint16_t>(col | (col << 8));
            *reinterpret_cast<uint16_t*>(row0 + x) = double_pixel;
            *reinterpret_cast<uint16_t*>(row1 + x) = double_pixel;
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
