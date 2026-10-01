#include "wormhole_effect.hh"

#include <cmath>
#include <euler/angles/degree.hh>

namespace demoscene {

wormhole_effect::wormhole_effect() {
    on_enter();
}

void wormhole_effect::on_enter() {
    // Reconstruct the deep twilight-to-electric-blue demoscene gradient
    m_canvas.clear(0);

    // Color ramp: 0 = black, 1..31 = deep blue to cyan-white highlight
    m_canvas.set_rgb(0, 0, 0, 0);
    for (int i = 1; i <= 31; ++i) {
        const float t = static_cast<float>(i) / 31.0f;
        const uint8_t r = static_cast<uint8_t>(std::pow(t, 2.5f) * 200.0f);
        const uint8_t g = static_cast<uint8_t>(std::pow(t, 1.5f) * 230.0f);
        const uint8_t b = static_cast<uint8_t>(t * 255.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
    }
}

void wormhole_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());

    // Interactive controls
    if (in.held(sdlpp::scancode::up)) m_speed = std::min(3.0f, m_speed + 1.0f * dt_sec);
    if (in.held(sdlpp::scancode::down)) m_speed = std::max(0.2f, m_speed - 1.0f * dt_sec);
    if (in.held(sdlpp::scancode::right)) m_sway_amp_x = std::min(100.0f, m_sway_amp_x + 20.0f * dt_sec);
    if (in.held(sdlpp::scancode::left)) m_sway_amp_x = std::max(10.0f, m_sway_amp_x - 20.0f * dt_sec);

    if (in.pressed(sdlpp::scancode::r)) {
        m_speed = 1.0f;
        m_sway_amp_x = 60.0f;
        m_sway_amp_y = 45.0f;
    }

    // Advance Lissajous orbit phases using euler::radian
    constexpr float omega_x = 2.4f;
    constexpr float omega_y = 3.0f;
    m_phase_x += euler::radian<float>(omega_x * m_speed * dt_sec);
    m_phase_y += euler::radian<float>(omega_y * m_speed * dt_sec);
}

void wormhole_effect::render(const neutrino::rect& viewport) {
    m_canvas.clear(0);

    constexpr float center_screen_x = 160.0f;
    constexpr float center_screen_y = 100.0f;

    // Render concentric tunnel rings from distant center (r=10) outward to near screen (r=220)
    float r = 10.0f;
    float r_step = 2.0f;
    uint8_t color_idx = 10;

    while (r < 220.0f) {
        // Compute center of this specific depth ring using euler::radian
        // Rings further down the tunnel lag behind in phase, creating the twist
        const float depth_factor = (220.0f - r) * 0.02f;
        const euler::radian<float> ring_angle_x = m_phase_x + euler::radian<float>(depth_factor);
        const euler::radian<float> ring_angle_y = m_phase_y + euler::radian<float>(depth_factor * 1.25f);

        const float ring_center_x = center_screen_x + m_sway_amp_x * std::cos(ring_angle_x.value());
        const float ring_center_y = center_screen_y + m_sway_amp_y * std::sin(ring_angle_y.value());

        // Sweep polar circle in steps
        constexpr int angle_step_deg = 4;
        for (int deg = 0; deg < 360; deg += angle_step_deg) {
            const euler::radian<float> theta = euler::degree<float>(static_cast<float>(deg));

            // Aspect ratio correction: Mode 13h had rectangular 5:6 pixels (320x200 on 4:3 CRT)
            const float aspect_scale_x = 1.15f;
            const int px = static_cast<int>(ring_center_x + r * aspect_scale_x * std::cos(theta.value()));
            const int py = static_cast<int>(ring_center_y + r * std::sin(theta.value()));

            m_canvas.put_pixel(px, py, color_idx);
        }

        r += r_step;
        r_step += 0.25f; // Rings widen exponentially as they approach the camera
        if (color_idx < 31) {
            color_idx++;
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
