#include "julia_effect.hh"

#include <algorithm>
#include <cmath>

namespace demoscene {

julia_effect::julia_effect() {
    on_enter();
}

void julia_effect::init_palette() {
    // 0: Deep space lake inside the Julia set
    m_canvas.set_rgb(0, 8, 6, 18);

    // 1..255: Multi-band psychedelic electric palette
    for (int i = 1; i <= 255; ++i) {
        const float t = static_cast<float>(i - 1) / 254.0f;
        // 3-cycle color wave
        const float phase = t * 6.283185f * 3.0f;
        const uint8_t r = static_cast<uint8_t>(std::clamp((std::sin(phase) + 1.0f) * 127.5f, 0.0f, 255.0f));
        const uint8_t g = static_cast<uint8_t>(std::clamp((std::sin(phase + 2.094f) + 1.0f) * 127.5f, 0.0f, 255.0f));
        const uint8_t b = static_cast<uint8_t>(std::clamp((std::sin(phase + 4.188f) + 1.0f) * 127.5f, 0.0f, 255.0f));
        m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
    }
}

void julia_effect::on_enter() {
    init_palette();
    m_center_x = 0.0f;
    m_center_y = 0.0f;
    m_zoom = 1.0f;
    m_c = euler::complex<float>{-0.7f, 0.27015f};
    m_animating_c = true;
    m_orbit_time = 0.0f;
    m_palette_timer = 0.0f;
}

void julia_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());

    // Continuous parameter morphing orbit
    if (m_animating_c) {
        m_orbit_time += dt_sec * 0.5f;
        m_c = euler::complex<float>{
            -0.8f + 0.16f * std::cos(m_orbit_time),
            0.156f + 0.14f * std::sin(m_orbit_time * 1.3f)
        };
    }

    // Interactive panning
    const float pan_step = (0.8f / m_zoom) * dt_sec;
    if (in.held(sdlpp::scancode::left))  m_center_x -= pan_step;
    if (in.held(sdlpp::scancode::right)) m_center_x += pan_step;
    if (in.held(sdlpp::scancode::up))    m_center_y -= pan_step;
    if (in.held(sdlpp::scancode::down))  m_center_y += pan_step;

    // Interactive zooming
    if (in.held(sdlpp::scancode::w)) m_zoom *= (1.0f + 1.5f * dt_sec);
    if (in.held(sdlpp::scancode::s)) m_zoom /= (1.0f + 1.5f * dt_sec);

    // Toggle continuous orbit animation
    if (in.pressed(sdlpp::scancode::space)) {
        m_animating_c = !m_animating_c;
    }

    // Classic Julia seed presets
    if (in.pressed(sdlpp::scancode::num_1)) {
        m_animating_c = false;
        m_c = euler::complex<float>{-0.7f, 0.27015f}; // Feathery Frost Dendrite
    }
    if (in.pressed(sdlpp::scancode::num_2)) {
        m_animating_c = false;
        m_c = euler::complex<float>{-0.4f, 0.6f};     // Douady's Classic Rabbit
    }
    if (in.pressed(sdlpp::scancode::num_3)) {
        m_animating_c = false;
        m_c = euler::complex<float>{-0.8f, 0.156f};   // Dragon & Cauliflower
    }
    if (in.pressed(sdlpp::scancode::num_4)) {
        m_animating_c = false;
        m_c = euler::complex<float>{0.285f, 0.01f};    // San Marco Cathedral
    }

    if (in.pressed(sdlpp::scancode::r)) {
        m_center_x = 0.0f;
        m_center_y = 0.0f;
        m_zoom = 1.0f;
        m_animating_c = true;
        m_orbit_time = 0.0f;
    }

    // Rotate palette in-place for fluid motion
    m_palette_timer += dt_sec;
    if (m_palette_timer >= 0.02f) {
        m_canvas.cycle_palette(1, 254, 1);
        m_palette_timer = 0.0f;
    }
}

void julia_effect::render(const neutrino::rect& viewport) {
    constexpr float aspect = static_cast<float>(vga_canvas::width) / static_cast<float>(vga_canvas::height);
    const float base_scale = 1.5f / (100.0f * m_zoom);
    const float step_x = base_scale * aspect;
    const float step_y = base_scale;

    for (int y = 0; y < vga_canvas::height; ++y) {
        const float c_im = m_center_y + (static_cast<float>(y) - 100.0f) * step_y;

        for (int x = 0; x < vga_canvas::width; ++x) {
            const float c_re = m_center_x + (static_cast<float>(x) - 160.0f) * step_x;

            euler::complex<float> z{c_re, c_im};
            int iter = 0;

            // Julia iteration loop: z_{n+1} = z_n^2 + c using euler::complex
            while (iter < max_iterations && z.norm() <= 4.0f) {
                z = z * z + m_c;
                ++iter;
            }

            if (iter >= max_iterations) {
                m_canvas.put_pixel_fast(x, y, 0); // Interior
            } else {
                // Map escape iteration count to dynamic palette index
                const auto color_idx = static_cast<uint8_t>(1 + (iter * 253 / max_iterations));
                m_canvas.put_pixel_fast(x, y, color_idx);
            }
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
