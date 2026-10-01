#include "fadeplot_effect.hh"

#include <algorithm>
#include <cmath>

namespace demoscene {

fadeplot_effect::fadeplot_effect() {
    on_enter();
}

void fadeplot_effect::init_palette() {
    m_canvas.set_rgb(0, 4, 6, 12); // CRT screen glass dark background

    switch (m_phosphor_theme) {
    case 0: {
        // P1 Classic Green CRT Phosphor:
        // Deep forest green -> Electric neon lime -> Pure white
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            uint8_t r = 0, g = 0, b = 0;
            if (t < 0.65f) {
                const float u = t / 0.65f;
                r = static_cast<uint8_t>(u * 40);
                g = static_cast<uint8_t>(20 + u * 235);
                b = static_cast<uint8_t>(u * 30);
            } else {
                const float u = (t - 0.65f) / 0.35f;
                r = static_cast<uint8_t>(40 + u * 215);
                g = 255;
                b = static_cast<uint8_t>(30 + u * 225);
            }
            m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
        }
        break;
    }
    case 1: {
        // P3 Retro Amber CRT Phosphor:
        // Deep mahogany -> Glowing warm amber -> Incandescent white
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            uint8_t r = 0, g = 0, b = 0;
            if (t < 0.65f) {
                const float u = t / 0.65f;
                r = static_cast<uint8_t>(40 + u * 215);
                g = static_cast<uint8_t>(10 + u * 150);
                b = static_cast<uint8_t>(u * 15);
            } else {
                const float u = (t - 0.65f) / 0.35f;
                r = 255;
                g = static_cast<uint8_t>(160 + u * 95);
                b = static_cast<uint8_t>(15 + u * 240);
            }
            m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
        }
        break;
    }
    case 2: {
        // Cyberpunk Ice Cyan Phosphor:
        // Deep indigo -> Electric cyan -> Blinding white
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            uint8_t r = 0, g = 0, b = 0;
            if (t < 0.65f) {
                const float u = t / 0.65f;
                r = static_cast<uint8_t>(u * 20);
                g = static_cast<uint8_t>(20 + u * 200);
                b = static_cast<uint8_t>(40 + u * 215);
            } else {
                const float u = (t - 0.65f) / 0.35f;
                r = static_cast<uint8_t>(20 + u * 235);
                g = static_cast<uint8_t>(220 + u * 35);
                b = 255;
            }
            m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
        }
        break;
    }
    default:
        break;
    }
}

void fadeplot_effect::on_enter() {
    init_palette();
    m_canvas.clear(0);
    m_orientation = euler::quaternion<float>::identity();
    m_angular_velocity = euler::vec3<float>{0.4f, 0.7f, 0.2f};
    m_zoom = 220.0f;
    m_time = 0.0f;
    m_curve_mode = 0;
    m_phosphor_theme = 0;
    m_decay_rate = 3;
}

euler::vec3<float> fadeplot_effect::evaluate_curve(float t) const {
    switch (m_curve_mode) {
    case 0: {
        // Trefoil Knot
        const float x = 25.0f * (std::sin(t) + 2.0f * std::sin(2.0f * t));
        const float y = 25.0f * (std::cos(t) - 2.0f * std::cos(2.0f * t));
        const float z = 25.0f * (-std::sin(3.0f * t));
        return euler::vec3<float>{x, y, z};
    }
    case 1: {
        // 3:4:5 Harmonic Lissajous Knot
        const float x = 65.0f * std::sin(3.0f * t);
        const float y = 60.0f * std::sin(4.0f * t + 0.7f);
        const float z = 60.0f * std::cos(5.0f * t + 1.2f);
        return euler::vec3<float>{x, y, z};
    }
    case 2: {
        // Torus Knot (p = 3, q = 5)
        const float r = 48.0f + 20.0f * std::cos(5.0f * t);
        const float x = r * std::cos(3.0f * t);
        const float y = r * std::sin(3.0f * t);
        const float z = 24.0f * std::sin(5.0f * t);
        return euler::vec3<float>{x, y, z};
    }
    case 3: {
        // 3D Lemniscate of Gerono (Figure-8)
        const float x = 65.0f * std::cos(t);
        const float y = 65.0f * std::sin(t) * std::cos(t);
        const float z = 40.0f * std::sin(2.0f * t);
        return euler::vec3<float>{x, y, z};
    }
    default:
        return euler::vec3<float>{0.0f, 0.0f, 0.0f};
    }
}

void fadeplot_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec * 1.5f;

    // Switch Knot Curve
    if (in.pressed(sdlpp::scancode::space)) {
        m_curve_mode = (m_curve_mode + 1) % 4;
        m_canvas.clear(0);
    }

    // Switch Phosphor Theme
    if (in.pressed(sdlpp::scancode::c)) {
        m_phosphor_theme = (m_phosphor_theme + 1) % 3;
        init_palette();
    }

    // Decay rate (persistence length)
    if (in.pressed(sdlpp::scancode::up))   m_decay_rate = std::max(1, m_decay_rate - 1);
    if (in.pressed(sdlpp::scancode::down)) m_decay_rate = std::min(8, m_decay_rate + 1);

    // Zoom Controls
    if (in.held(sdlpp::scancode::w)) m_zoom = std::min(400.0f, m_zoom + 100.0f * dt_sec);
    if (in.held(sdlpp::scancode::s)) m_zoom = std::max(100.0f, m_zoom - 100.0f * dt_sec);

    // Interactive Manual Rotation Bias
    if (in.held(sdlpp::scancode::left))  m_angular_velocity[1] -= 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::right)) m_angular_velocity[1] += 2.0f * dt_sec;

    if (in.pressed(sdlpp::scancode::r)) {
        m_orientation = euler::quaternion<float>::identity();
        m_angular_velocity = euler::vec3<float>{0.4f, 0.7f, 0.2f};
        m_zoom = 220.0f;
        m_decay_rate = 3;
        m_curve_mode = 0;
        m_canvas.clear(0);
    }

    // Increment 3D rotation using euler::quaternion
    const float speed = m_angular_velocity.length();
    if (speed > 1e-4f) {
        const euler::vec3<float> axis = m_angular_velocity.normalized();
        const euler::radian<float> angle_delta = euler::radian<float>(speed * dt_sec);
        const euler::quaternion<float> delta_rot = euler::quaternion<float>::from_axis_angle(axis, angle_delta);
        m_orientation = (delta_rot * m_orientation).normalized();
    }
}

void fadeplot_effect::render(const neutrino::rect& viewport) {
    constexpr float camera_z = 200.0f;

    // 1. Simulate CRT phosphor decay across existing pixels
    uint8_t* raw = m_canvas.raw_pixels();
    const uint8_t decay = static_cast<uint8_t>(m_decay_rate);

    for (int i = 0; i < vga_canvas::pixel_count; ++i) {
        if (raw[i] > 0) {
            raw[i] = (raw[i] > decay) ? (raw[i] - decay) : 0;
        }
    }

    // 2. Plot new incandescent particle trail along the parametric space curve
    constexpr int sample_steps = 160;
    constexpr float step_dt = 0.007f;

    for (int i = 0; i < sample_steps; ++i) {
        const float t = m_time + static_cast<float>(i) * step_dt;
        const euler::vec3<float> local_pos = evaluate_curve(t);

        // 3D Quaternion rotation
        const euler::vec3<float> rot_pos = m_orientation.rotate(local_pos);

        // Perspective divide
        const float pz = rot_pos.z() + camera_z;
        if (pz <= 1.0f) continue;
        const float inv_z = m_zoom / pz;

        const int sx = 160 + static_cast<int>(rot_pos.x() * inv_z);
        const int sy = 100 + static_cast<int>(rot_pos.y() * inv_z);

        // Plot glowing core and soft halo
        m_canvas.put_pixel(sx, sy, 255); // Blinding specular tip
        m_canvas.put_pixel(sx + 1, sy, 230);
        m_canvas.put_pixel(sx - 1, sy, 230);
        m_canvas.put_pixel(sx, sy + 1, 230);
        m_canvas.put_pixel(sx, sy - 1, 230);
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
