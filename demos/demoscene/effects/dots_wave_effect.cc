#include "dots_wave_effect.hh"

#include <algorithm>
#include <cmath>

namespace demoscene {

dots_wave_effect::dots_wave_effect() {
    on_enter();
}

void dots_wave_effect::init_palette() {
    m_canvas.set_rgb(0, 6, 8, 16); // Deep space background

    switch (m_color_theme) {
    case 0: {
        // Ice Cyan / Electric Blue:
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            uint8_t r = 0, g = 0, b = 0;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                r = static_cast<uint8_t>(u * 20);
                g = static_cast<uint8_t>(30 + u * 170);
                b = static_cast<uint8_t>(60 + u * 195);
            } else {
                const float u = (t - 0.6f) / 0.4f;
                r = static_cast<uint8_t>(20 + u * 235);
                g = static_cast<uint8_t>(200 + u * 55);
                b = 255;
            }
            m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
        }
        break;
    }
    case 1: {
        // Synthwave Hot Pink / Magenta:
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            uint8_t r = 0, g = 0, b = 0;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                r = static_cast<uint8_t>(50 + u * 190);
                g = static_cast<uint8_t>(u * 20);
                b = static_cast<uint8_t>(60 + u * 140);
            } else {
                const float u = (t - 0.6f) / 0.4f;
                r = 255;
                g = static_cast<uint8_t>(20 + u * 235);
                b = static_cast<uint8_t>(200 + u * 55);
            }
            m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
        }
        break;
    }
    case 2: {
        // Matrix Emerald Green:
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            uint8_t r = 0, g = 0, b = 0;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                r = static_cast<uint8_t>(u * 30);
                g = static_cast<uint8_t>(40 + u * 200);
                b = static_cast<uint8_t>(u * 40);
            } else {
                const float u = (t - 0.6f) / 0.4f;
                r = static_cast<uint8_t>(30 + u * 225);
                g = 255;
                b = static_cast<uint8_t>(40 + u * 215);
            }
            m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
        }
        break;
    }
    default:
        break;
    }
}

void dots_wave_effect::on_enter() {
    init_palette();
    m_canvas.clear(0);
    m_orientation = euler::quaternion<float>::identity();
    m_angular_velocity = euler::vec3<float>{0.4f, 0.7f, 0.2f};
    m_zoom = 220.0f;
    m_time = 0.0f;
    m_pattern_mode = 0;
    m_color_theme = 0;
}

euler::vec3<float> dots_wave_effect::compute_dot_position(int index, float t) const {
    switch (m_pattern_mode) {
    case 0: {
        // Undulating Multi-Harmonic Ribbon Wave (DOTS1.PAS / DOTS2.PAS)
        const float u = static_cast<float>(index) * 0.018f;
        const float x = 85.0f * std::sin(u * 1.6f + t * 2.2f) + 30.0f * std::cos(u * 2.8f - t * 1.4f);
        const float y = 60.0f * std::sin(u * 2.1f - t * 1.8f) + 25.0f * std::sin(u * 3.5f + t * 2.5f);
        const float z = 60.0f * std::cos(u * 1.8f + t * 1.6f);
        return euler::vec3<float>{x, y, z};
    }
    case 1: {
        // Rotating Torus Ring Vortex
        const float u = static_cast<float>(index) * 0.038f;
        const float v = u * 3.0f + t * 2.2f;
        const float r = 55.0f + 22.0f * std::cos(v);
        const float x = r * std::cos(u);
        const float y = r * std::sin(u);
        const float z = 25.0f * std::sin(v);
        return euler::vec3<float>{x, y, z};
    }
    case 2: {
        // 3D Cube Matrix Grid (9x9x9 = 729 points with spherical wave ripple)
        const int idx = index % 729;
        const int ix = (idx % 9) - 4;
        const int iy = ((idx / 9) % 9) - 4;
        const int iz = (idx / 81) - 4;

        const float dx = static_cast<float>(ix) * 14.0f;
        const float dy = static_cast<float>(iy) * 14.0f;
        const float dz = static_cast<float>(iz) * 14.0f;

        const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        const float wave = std::sin(dist * 0.09f - t * 3.5f) * 12.0f;
        const float inv_d = (dist > 1e-4f) ? (1.0f / dist) : 0.0f;

        return euler::vec3<float>{
            dx + dx * inv_d * wave,
            dy + dy * inv_d * wave,
            dz + dz * inv_d * wave
        };
    }
    default:
        return euler::vec3<float>{0.0f, 0.0f, 0.0f};
    }
}

void dots_wave_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    // Switch Pattern
    if (in.pressed(sdlpp::scancode::space)) {
        m_pattern_mode = (m_pattern_mode + 1) % 3;
        m_canvas.clear(0);
    }

    // Switch Color Theme
    if (in.pressed(sdlpp::scancode::c)) {
        m_color_theme = (m_color_theme + 1) % 3;
        init_palette();
    }

    // Zoom Controls
    if (in.held(sdlpp::scancode::w)) m_zoom = std::min(400.0f, m_zoom + 100.0f * dt_sec);
    if (in.held(sdlpp::scancode::s)) m_zoom = std::max(100.0f, m_zoom - 100.0f * dt_sec);

    // Interactive Manual Rotation Bias
    if (in.held(sdlpp::scancode::left))  m_angular_velocity[1] -= 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::right)) m_angular_velocity[1] += 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::up))    m_angular_velocity[0] -= 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::down))  m_angular_velocity[0] += 2.0f * dt_sec;

    if (in.pressed(sdlpp::scancode::r)) {
        m_orientation = euler::quaternion<float>::identity();
        m_angular_velocity = euler::vec3<float>{0.4f, 0.7f, 0.2f};
        m_zoom = 220.0f;
        m_pattern_mode = 0;
        m_color_theme = 0;
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

void dots_wave_effect::render(const neutrino::rect& viewport) {
    constexpr float camera_z = 200.0f;

    // 1. Phosphor decay across existing canvas buffer
    uint8_t* raw = m_canvas.raw_pixels();
    for (int i = 0; i < vga_canvas::pixel_count; ++i) {
        if (raw[i] > 0) {
            raw[i] = (raw[i] > 4) ? (raw[i] - 4) : 0;
        }
    }

    // 2. Evaluate and plot 850 dots with depth-cued color and perspective scaling
    for (int i = 0; i < num_dots; ++i) {
        const euler::vec3<float> local_pos = compute_dot_position(i, m_time);
        const euler::vec3<float> rot_pos = m_orientation.rotate(local_pos);

        const float pz = rot_pos.z() + camera_z;
        if (pz <= 1.0f) continue;
        const float inv_z = m_zoom / pz;

        const int sx = 160 + static_cast<int>(rot_pos.x() * inv_z);
        const int sy = 100 + static_cast<int>(rot_pos.y() * inv_z);

        // Depth cueing: Closer dots are brighter and larger
        const int color_val = std::clamp(static_cast<int>(180.0f + (rot_pos.z() * 1.1f)), 40, 255);
        const uint8_t color_idx = static_cast<uint8_t>(color_val);

        m_canvas.put_pixel(sx, sy, color_idx);

        if (pz < 170.0f) {
            // Near plane magnification: plot 2x2 dot with soft halo
            m_canvas.put_pixel(sx + 1, sy, static_cast<uint8_t>(color_idx * 0.75f));
            m_canvas.put_pixel(sx, sy + 1, static_cast<uint8_t>(color_idx * 0.75f));
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
