#include "moire_effect.hh"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace demoscene {

namespace {

constexpr int k_lut_size = 1024;
constexpr int k_lut_mask = k_lut_size - 1;

const auto& get_sine_lut() {
    static const auto lut = [] {
        std::array<float, k_lut_size> table{};
        constexpr float pi2 = static_cast<float>(std::numbers::pi * 2.0);
        for (int i = 0; i < k_lut_size; ++i) {
            table[i] = std::sin(static_cast<float>(i) / static_cast<float>(k_lut_size) * pi2);
        }
        return table;
    }();
    return lut;
}

} // namespace

moire_effect::moire_effect() {
    on_enter();
}

void moire_effect::on_enter() {
    m_canvas.clear(0);
    init_palette();
    m_frequency = 1.0f;
    m_manual_control = false;
    m_time = 0.0f;
}

void moire_effect::init_palette() {
    m_canvas.set_rgb_6bit(0, 0, 0, 0);

    switch (m_theme) {
    case 0: { // Monochrome High-Contrast Opal / Silver
        m_canvas.set_rgb(0, 5, 8, 15);
        m_canvas.set_rgb(1, 240, 245, 255);
        for (int i = 2; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            const uint8_t c = static_cast<uint8_t>(t * 255.0f);
            m_canvas.set_rgb(static_cast<uint8_t>(i), c, c, static_cast<uint8_t>(std::min(255, c + 20)));
        }
        break;
    }
    case 1: { // Electric Rainbow Spectrum
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
    case 2: { // Synthwave Neon (Laser Violet -> Electric Magenta -> Cyan)
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.5f) {
                const float u = t / 0.5f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(u * 240.0f),
                    static_cast<uint8_t>(u * 20.0f),
                    static_cast<uint8_t>(100.0f + u * 155.0f));
            } else {
                const float u = (t - 0.5f) / 0.5f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(240.0f - u * 240.0f),
                    static_cast<uint8_t>(20.0f + u * 225.0f),
                    255);
            }
        }
        break;
    }
    case 3: { // Thermal Infrared FLIR
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(std::clamp(t * 3.0f, 0.0f, 1.0f) * 255.0f),
                static_cast<uint8_t>(std::clamp(t * 3.0f - 1.0f, 0.0f, 1.0f) * 255.0f),
                static_cast<uint8_t>(std::clamp(t * 3.0f - 2.0f, 0.0f, 1.0f) * 255.0f));
        }
        break;
    }
    }
}

void moire_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    if (in.pressed(sdlpp::scancode::space)) {
        m_mode = (m_mode + 1) % 3;
    }

    if (in.pressed(sdlpp::scancode::c)) {
        m_theme = (m_theme + 1) % 4;
        init_palette();
    }

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }

    if (in.held(sdlpp::scancode::equals) || in.held(sdlpp::scancode::kp_plus)) {
        m_frequency = std::min(2.5f, m_frequency + 0.6f * dt_sec);
    }
    if (in.held(sdlpp::scancode::minus) || in.held(sdlpp::scancode::kp_minus)) {
        m_frequency = std::max(0.4f, m_frequency - 0.6f * dt_sec);
    }

    // Manual control over center 1
    if (in.held(sdlpp::scancode::left))  { m_cx1 = std::max(10.0f, m_cx1 - 120.0f * dt_sec); m_manual_control = true; }
    if (in.held(sdlpp::scancode::right)) { m_cx1 = std::min(310.0f, m_cx1 + 120.0f * dt_sec); m_manual_control = true; }
    if (in.held(sdlpp::scancode::up))    { m_cy1 = std::max(10.0f, m_cy1 - 100.0f * dt_sec); m_manual_control = true; }
    if (in.held(sdlpp::scancode::down))  { m_cy1 = std::min(190.0f, m_cy1 + 100.0f * dt_sec); m_manual_control = true; }

    // Natural Lissajous wave orbit motion
    const float t = m_time;
    if (!m_manual_control) {
        m_cx1 = 160.0f + std::sin(t * 1.1f) * 95.0f;
        m_cy1 = 100.0f + std::sin(t * 0.7f) * 55.0f;
    }

    m_cx2 = 160.0f + std::cos(t * 0.9f) * 95.0f;
    m_cy2 = 100.0f + std::cos(t * 1.3f) * 55.0f;

    m_cx3 = 160.0f + std::sin(t * 1.5f + 1.2f) * 75.0f;
    m_cy3 = 100.0f + std::cos(t * 1.7f + 0.8f) * 45.0f;
}

void moire_effect::render(const neutrino::rect& viewport) {
    uint8_t* raw = m_canvas.raw_pixels();
    const auto& sin_lut = get_sine_lut();

    const float cx1 = m_cx1, cy1 = m_cy1;
    const float cx2 = m_cx2, cy2 = m_cy2;
    const float cx3 = m_cx3, cy3 = m_cy3;
    const float freq = m_frequency;

    // Mode 0: High-Contrast Flat XOR Moiré Fringes
    if (m_mode == 0) {
        for (int y = 0; y < vga_canvas::height; ++y) {
            const float dy1 = static_cast<float>(y) - cy1;
            const float dy2 = static_cast<float>(y) - cy2;
            const float dy1_sq = dy1 * dy1;
            const float dy2_sq = dy2 * dy2;
            uint8_t* row = raw + y * vga_canvas::width;

            for (int x = 0; x < vga_canvas::width; ++x) {
                const float dx1 = static_cast<float>(x) - cx1;
                const float dx2 = static_cast<float>(x) - cx2;

                const int r1 = static_cast<int>(std::sqrt(dx1 * dx1 + dy1_sq) * freq);
                const int r2 = static_cast<int>(std::sqrt(dx2 * dx2 + dy2_sq) * freq);

                const uint8_t val = ((r1 ^ r2) >> 4) & 1 ? 1 : 0;
                row[x] = val;
            }
        }
    }
    // Mode 1: Continuous Multi-Frequency Wave Interference
    else if (m_mode == 1) {
        constexpr float lut_scale = static_cast<float>(k_lut_size) / 28.0f;

        for (int y = 0; y < vga_canvas::height; ++y) {
            const float dy1 = static_cast<float>(y) - cy1;
            const float dy2 = static_cast<float>(y) - cy2;
            const float dy1_sq = dy1 * dy1;
            const float dy2_sq = dy2 * dy2;
            uint8_t* row = raw + y * vga_canvas::width;

            for (int x = 0; x < vga_canvas::width; ++x) {
                const float dx1 = static_cast<float>(x) - cx1;
                const float dx2 = static_cast<float>(x) - cx2;

                const float r1 = std::sqrt(dx1 * dx1 + dy1_sq) * freq;
                const float r2 = std::sqrt(dx2 * dx2 + dy2_sq) * freq;

                const int idx1 = static_cast<int>(r1 * lut_scale) & k_lut_mask;
                const int idx2 = static_cast<int>(r2 * (lut_scale * 1.33f)) & k_lut_mask;

                const float wave = (sin_lut[idx1] + sin_lut[idx2]) * 0.5f;
                const int col = static_cast<int>((wave + 1.0f) * 125.0f) + 4;
                row[x] = static_cast<uint8_t>(std::clamp(col, 1, 255));
            }
        }
    }
    // Mode 2: 3-Center Sierpinski Radial Lattice
    else {
        for (int y = 0; y < vga_canvas::height; ++y) {
            const float dy1 = static_cast<float>(y) - cy1;
            const float dy2 = static_cast<float>(y) - cy2;
            const float dy3 = static_cast<float>(y) - cy3;
            const float dy1_sq = dy1 * dy1;
            const float dy2_sq = dy2 * dy2;
            const float dy3_sq = dy3 * dy3;
            uint8_t* row = raw + y * vga_canvas::width;

            for (int x = 0; x < vga_canvas::width; ++x) {
                const float dx1 = static_cast<float>(x) - cx1;
                const float dx2 = static_cast<float>(x) - cx2;
                const float dx3 = static_cast<float>(x) - cx3;

                const int r1 = static_cast<int>(std::sqrt(dx1 * dx1 + dy1_sq) * freq);
                const int r2 = static_cast<int>(std::sqrt(dx2 * dx2 + dy2_sq) * freq);
                const int r3 = static_cast<int>(std::sqrt(dx3 * dx3 + dy3_sq) * freq);

                const int xor_val = ((r1 ^ r2 ^ r3) >> 3) & 0x1F;
                row[x] = static_cast<uint8_t>(xor_val * 8 + 4);
            }
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
