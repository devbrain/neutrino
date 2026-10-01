#include "vectorballs_effect.hh"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace demoscene {

vectorballs_effect::vectorballs_effect() {
    build_shape(0);
    on_enter();
}

void vectorballs_effect::build_shape(int shape_idx) {
    m_balls.clear();

    switch (shape_idx) {
    case 0: {
        // 3D Hollow Cube (3x3x3 surface balls, 26 balls)
        constexpr float spacing = 38.0f;
        constexpr float ball_r = 11.0f;
        for (int z = -1; z <= 1; ++z) {
            for (int y = -1; y <= 1; ++y) {
                for (int x = -1; x <= 1; ++x) {
                    if (x == 0 && y == 0 && z == 0) continue; // Hollow center
                    m_balls.push_back({
                        euler::vec3<float>{static_cast<float>(x) * spacing,
                                          static_cast<float>(y) * spacing,
                                          static_cast<float>(z) * spacing},
                        ball_r
                    });
                }
            }
        }
        break;
    }
    case 1: {
        // 3D Double Helix / DNA Strand (64 balls)
        constexpr int pairs = 32;
        constexpr float ball_r = 9.0f;
        constexpr float radius = 42.0f;
        constexpr float height_span = 140.0f;

        for (int i = 0; i < pairs; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(pairs - 1);
            const float y = (t - 0.5f) * height_span;
            const float angle = t * 4.0f * std::numbers::pi_v<float>;

            // Strand A
            m_balls.push_back({
                euler::vec3<float>{radius * std::cos(angle), y, radius * std::sin(angle)},
                ball_r
            });

            // Strand B
            m_balls.push_back({
                euler::vec3<float>{radius * std::cos(angle + std::numbers::pi_v<float>), y, radius * std::sin(angle + std::numbers::pi_v<float>)},
                ball_r
            });
        }
        break;
    }
    case 2: {
        // 3D Torus Trefoil Knot (60 balls)
        constexpr int count = 60;
        constexpr float ball_r = 10.0f;

        for (int i = 0; i < count; ++i) {
            const float phi = static_cast<float>(i) * (2.0f * std::numbers::pi_v<float> / static_cast<float>(count));
            const float r = 50.0f + 20.0f * std::cos(3.0f * phi);
            const float x = r * std::cos(2.0f * phi);
            const float y = r * std::sin(2.0f * phi);
            const float z = 25.0f * std::sin(3.0f * phi);

            m_balls.push_back({euler::vec3<float>{x, y, z}, ball_r});
        }
        break;
    }
    default:
        break;
    }
}

void vectorballs_effect::init_palette() {
    m_canvas.set_rgb(0, 6, 8, 16); // Deep space background

    switch (m_color_theme) {
    case 0: {
        // Metallic Chrome:
        // Deep steel blue -> Bright silver -> Blinding white specular
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            uint8_t r = 0, g = 0, b = 0;
            if (t < 0.65f) {
                const float u = t / 0.65f;
                r = static_cast<uint8_t>(30 + u * 150);
                g = static_cast<uint8_t>(40 + u * 165);
                b = static_cast<uint8_t>(60 + u * 195);
            } else {
                const float u = (t - 0.65f) / 0.35f;
                r = static_cast<uint8_t>(180 + u * 75);
                g = static_cast<uint8_t>(205 + u * 50);
                b = 255;
            }
            m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
        }
        break;
    }
    case 1: {
        // Synthwave Hot Magenta & Cyan:
        // Deep purple -> Electric magenta -> Ice white
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            uint8_t r = 0, g = 0, b = 0;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                r = static_cast<uint8_t>(70 + u * 170);
                g = static_cast<uint8_t>(u * 40);
                b = static_cast<uint8_t>(90 + u * 120);
            } else {
                const float u = (t - 0.6f) / 0.4f;
                r = 255;
                g = static_cast<uint8_t>(40 + u * 215);
                b = static_cast<uint8_t>(210 + u * 45);
            }
            m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
        }
        break;
    }
    case 2: {
        // Emerald Gold:
        // Deep moss green -> Vivid jade -> Radiant solar gold
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            uint8_t r = 0, g = 0, b = 0;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                r = static_cast<uint8_t>(u * 90);
                g = static_cast<uint8_t>(50 + u * 170);
                b = static_cast<uint8_t>(20 + u * 60);
            } else {
                const float u = (t - 0.6f) / 0.4f;
                r = static_cast<uint8_t>(90 + u * 165);
                g = 255;
                b = static_cast<uint8_t>(80 + u * 175);
            }
            m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
        }
        break;
    }
    default:
        break;
    }
}

void vectorballs_effect::on_enter() {
    init_palette();
    m_canvas.clear(0);
    m_orientation = euler::quaternion<float>::identity();
    m_angular_velocity = euler::vec3<float>{0.6f, 0.9f, 0.4f};
    m_zoom = 220.0f;
    m_time = 0.0f;
    m_shape_idx = 0;
    m_color_theme = 0;
    build_shape(0);
}

void vectorballs_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    // Switch Vectorball Shape
    if (in.pressed(sdlpp::scancode::space)) {
        m_shape_idx = (m_shape_idx + 1) % 3;
        build_shape(m_shape_idx);
    }

    // Switch Palette Theme
    if (in.pressed(sdlpp::scancode::c)) {
        m_color_theme = (m_color_theme + 1) % 3;
        init_palette();
    }

    // Zoom Controls
    if (in.held(sdlpp::scancode::w)) m_zoom = std::min(400.0f, m_zoom + 100.0f * dt_sec);
    if (in.held(sdlpp::scancode::s)) m_zoom = std::max(100.0f, m_zoom - 100.0f * dt_sec);

    // Interactive Rotation Bias
    if (in.held(sdlpp::scancode::left))  m_angular_velocity[1] -= 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::right)) m_angular_velocity[1] += 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::up))    m_angular_velocity[0] -= 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::down))  m_angular_velocity[0] += 2.0f * dt_sec;

    if (in.pressed(sdlpp::scancode::r)) {
        m_orientation = euler::quaternion<float>::identity();
        m_angular_velocity = euler::vec3<float>{0.6f, 0.9f, 0.4f};
        m_zoom = 220.0f;
        m_shape_idx = 0;
        m_color_theme = 0;
        build_shape(0);
        init_palette();
    }

    // Update quaternion orientation
    const float speed = m_angular_velocity.length();
    if (speed > 1e-4f) {
        const euler::vec3<float> axis = m_angular_velocity.normalized();
        const euler::radian<float> angle_delta = euler::radian<float>(speed * dt_sec);
        const euler::quaternion<float> delta_rot = euler::quaternion<float>::from_axis_angle(axis, angle_delta);
        m_orientation = (delta_rot * m_orientation).normalized();
    }
}

void vectorballs_effect::draw_shaded_ball(int cx, int cy, float radius, uint8_t base_col) {
    const int ir = static_cast<int>(radius);
    if (ir <= 0) return;

    constexpr float lx = -0.45f;
    constexpr float ly = -0.55f;
    constexpr float lz = 0.70f;

    const float inv_r = 1.0f / radius;

    for (int dy = -ir; dy <= ir; ++dy) {
        const int py = cy + dy;
        if (py < 0 || py >= vga_canvas::height) continue;

        const float ny = static_cast<float>(dy) * inv_r;
        const float ny2 = ny * ny;

        for (int dx = -ir; dx <= ir; ++dx) {
            const int px = cx + dx;
            if (px < 0 || px >= vga_canvas::width) continue;

            const float nx = static_cast<float>(dx) * inv_r;
            const float r2 = nx * nx + ny2;

            if (r2 <= 1.0f) {
                const float nz = std::sqrt(1.0f - r2);

                // Lambertian diffuse
                const float diffuse = std::max(0.0f, nx * lx + ny * ly + nz * lz);

                // Blinn-Phong specular highlight
                const float hx = lx;
                const float hy = ly;
                const float hz = lz + 1.0f;
                const float h_len = std::sqrt(hx * hx + hy * hy + hz * hz);
                const float spec_dot = std::max(0.0f, (nx * hx + ny * hy + nz * hz) / h_len);
                const float specular = std::pow(spec_dot, 14.0f);

                const float intensity = std::clamp(0.20f + 0.60f * diffuse + 0.45f * specular, 0.0f, 1.0f);
                const uint8_t color = static_cast<uint8_t>(base_col + static_cast<int>(intensity * 230.0f));

                m_canvas.put_pixel_fast(px, py, color);
            }
        }
    }
}

void vectorballs_effect::render(const neutrino::rect& viewport) {
    m_canvas.clear(0);

    constexpr float camera_z = 210.0f;

    struct projected_ball {
        int sx, sy;
        float screen_radius;
        float depth;
    };

    std::vector<projected_ball> proj_balls;
    proj_balls.reserve(m_balls.size());

    // 1. Rotate each ball with quaternion and project with perspective
    for (const auto& b : m_balls) {
        const euler::vec3<float> rot_pos = m_orientation.rotate(b.local_pos);

        const float pz = rot_pos.z() + camera_z;
        if (pz <= 1.0f) continue;
        const float inv_z = m_zoom / pz;

        const int sx = 160 + static_cast<int>(rot_pos.x() * inv_z);
        const int sy = 100 + static_cast<int>(rot_pos.y() * inv_z);
        const float sr = b.radius * inv_z;

        proj_balls.push_back({sx, sy, sr, rot_pos.z()});
    }

    // 2. Painter's Algorithm Depth Sort: Render from furthest to nearest
    std::sort(proj_balls.begin(), proj_balls.end(), [](const projected_ball& a, const projected_ball& b) {
        return a.depth > b.depth;
    });

    // 3. Rasterize shaded 3D spheres
    for (const auto& pb : proj_balls) {
        draw_shaded_ball(pb.sx, pb.sy, pb.screen_radius, 20);
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
