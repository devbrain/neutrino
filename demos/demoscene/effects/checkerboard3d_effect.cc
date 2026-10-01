#include "checkerboard3d_effect.hh"

#include <algorithm>
#include <cmath>

namespace demoscene {

checkerboard3d_effect::checkerboard3d_effect() {
    build_mesh();
    on_enter();
}

void checkerboard3d_effect::build_mesh() {
    m_base_vertices.clear();
    m_faces.clear();

    constexpr float step = 14.0f;
    constexpr float half_span = (static_cast<float>(grid_size - 1) * step) * 0.5f;

    // Generate 7x7 vertex grid in the XY plane
    for (int j = 0; j < grid_size; ++j) {
        for (int i = 0; i < grid_size; ++i) {
            const float x = static_cast<float>(i) * step - half_span;
            const float y = static_cast<float>(j) * step - half_span;
            m_base_vertices.push_back(euler::vec3<float>{x, y, 0.0f});
        }
    }

    // Generate 6x6 = 36 quad faces with alternating checkerboard pattern
    for (int j = 0; j < grid_size - 1; ++j) {
        for (int i = 0; i < grid_size - 1; ++i) {
            const size_t v0 = static_cast<size_t>(j * grid_size + i);
            const size_t v1 = static_cast<size_t>(j * grid_size + (i + 1));
            const size_t v2 = static_cast<size_t>((j + 1) * grid_size + (i + 1));
            const size_t v3 = static_cast<size_t>((j + 1) * grid_size + i);

            const bool is_white = ((i + j) & 1) == 0;
            m_faces.push_back({v0, v1, v2, v3, is_white, 0.0f});
        }
    }
}

void checkerboard3d_effect::init_palette() {
    // 0: Deep space backdrop
    m_canvas.set_rgb(0, 8, 10, 20);

    // 1..40: Dark tile ramp (Theme 0)
    for (int i = 1; i <= 40; ++i) {
        const float t = static_cast<float>(i) / 40.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(20 + t * 40),
            static_cast<uint8_t>(25 + t * 45),
            static_cast<uint8_t>(35 + t * 55));
    }

    // 41..80: Light tile ramp (Theme 0: Ivory / Cream)
    for (int i = 41; i <= 80; ++i) {
        const float t = static_cast<float>(i - 41) / 39.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(140 + t * 115),
            static_cast<uint8_t>(140 + t * 115),
            static_cast<uint8_t>(125 + t * 110));
    }

    // 81..120: Cyberpunk Neon Cyan ramp (Theme 1)
    for (int i = 81; i <= 120; ++i) {
        const float t = static_cast<float>(i - 81) / 39.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(t * 40),
            static_cast<uint8_t>(120 + t * 135),
            static_cast<uint8_t>(160 + t * 95));
    }

    // 121..160: Cyberpunk Hot Magenta ramp (Theme 1)
    for (int i = 121; i <= 160; ++i) {
        const float t = static_cast<float>(i - 121) / 39.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(160 + t * 95),
            static_cast<uint8_t>(t * 30),
            static_cast<uint8_t>(120 + t * 100));
    }

    // 161..200: Emerald Green ramp (Theme 2)
    for (int i = 161; i <= 200; ++i) {
        const float t = static_cast<float>(i - 161) / 39.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(t * 30),
            static_cast<uint8_t>(140 + t * 115),
            static_cast<uint8_t>(70 + t * 80));
    }

    // 201..240: Brilliant Gold ramp (Theme 2)
    for (int i = 201; i <= 240; ++i) {
        const float t = static_cast<float>(i - 201) / 39.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(180 + t * 75),
            static_cast<uint8_t>(140 + t * 100),
            static_cast<uint8_t>(20 + t * 40));
    }
}

void checkerboard3d_effect::on_enter() {
    init_palette();
    m_orientation = euler::quaternion<float>::identity();
    m_angular_velocity = euler::vec3<float>{0.6f, 0.9f, 0.4f};
    m_zoom = 220.0f;
    m_time = 0.0f;
    m_wave_mode = 1;
    m_color_theme = 0;
}

void checkerboard3d_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    // Interactive Model Wave Mode
    if (in.pressed(sdlpp::scancode::space)) {
        m_wave_mode = (m_wave_mode + 1) % 3;
    }

    // Color Theme Switch
    if (in.pressed(sdlpp::scancode::c)) {
        m_color_theme = (m_color_theme + 1) % 3;
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
        m_angular_velocity = euler::vec3<float>{0.6f, 0.9f, 0.4f};
        m_zoom = 220.0f;
        m_wave_mode = 1;
        m_color_theme = 0;
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

void checkerboard3d_effect::rasterize_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color) {
    // Sort vertices by Y: y0 <= y1 <= y2
    if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); }
    if (y0 > y2) { std::swap(x0, x2); std::swap(y0, y2); }
    if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }

    if (y0 == y2 || y2 < 0 || y0 >= vga_canvas::height) return;

    const int total_height = y2 - y0;

    for (int y = y0; y <= y2; ++y) {
        if (y < 0 || y >= vga_canvas::height) continue;

        const bool second_half = (y > y1) || (y1 == y0);
        const int segment_height = second_half ? (y2 - y1) : (y1 - y0);
        if (segment_height == 0) continue;

        const float alpha = static_cast<float>(y - y0) / static_cast<float>(total_height);
        const float beta = static_cast<float>(y - (second_half ? y1 : y0)) / static_cast<float>(segment_height);

        int xa = static_cast<int>(static_cast<float>(x0) + static_cast<float>(x2 - x0) * alpha);
        int xb = second_half
            ? static_cast<int>(static_cast<float>(x1) + static_cast<float>(x2 - x1) * beta)
            : static_cast<int>(static_cast<float>(x0) + static_cast<float>(x1 - x0) * beta);

        if (xa > xb) std::swap(xa, xb);

        const int cl_start = std::max(0, xa);
        const int cl_end = std::min(vga_canvas::width - 1, xb);

        for (int x = cl_start; x <= cl_end; ++x) {
            m_canvas.put_pixel_fast(x, y, color);
        }
    }
}

void checkerboard3d_effect::render(const neutrino::rect& viewport) {
    m_canvas.clear(0);

    constexpr float camera_z = 180.0f;
    const euler::vec3<float> light_dir = euler::vec3<float>{0.4f, 0.7f, -1.0f}.normalized();

    // 1. Calculate dynamic wave deformation & rotate vertices
    std::vector<euler::vec3<float>> rotated_vertices;
    rotated_vertices.reserve(vertex_count);

    std::vector<int> screen_x(vertex_count);
    std::vector<int> screen_y(vertex_count);

    for (size_t idx = 0; idx < m_base_vertices.size(); ++idx) {
        const auto& base = m_base_vertices[idx];

        // Wave deformation
        float z_disp = 0.0f;
        if (m_wave_mode == 1) {
            z_disp = 16.0f * std::sin(base.x() * 0.07f + m_time * 3.2f) * std::cos(base.y() * 0.07f + m_time * 2.4f);
        } else if (m_wave_mode == 2) {
            z_disp = 12.0f * std::sin((base.x() * base.y()) * 0.002f + m_time * 2.8f);
        }

        const euler::vec3<float> local_pos{base.x(), base.y(), z_disp};
        const euler::vec3<float> rot_pos = m_orientation.rotate(local_pos);
        rotated_vertices.push_back(rot_pos);

        // Perspective projection
        const float pz = rot_pos.z() + camera_z;
        const float inv_z = (pz > 1.0f) ? (m_zoom / pz) : 1.0f;

        screen_x[idx] = 160 + static_cast<int>(rot_pos.x() * inv_z);
        screen_y[idx] = 100 + static_cast<int>(rot_pos.y() * inv_z);
    }

    // 2. Compute face depth for Painter's Algorithm sorting
    for (auto& face : m_faces) {
        face.depth = (rotated_vertices[face.v0].z() +
                      rotated_vertices[face.v1].z() +
                      rotated_vertices[face.v2].z() +
                      rotated_vertices[face.v3].z()) * 0.25f;
    }

    // Sort faces from back to front (largest depth first)
    std::vector<size_t> sorted_indices(m_faces.size());
    for (size_t i = 0; i < sorted_indices.size(); ++i) sorted_indices[i] = i;

    std::sort(sorted_indices.begin(), sorted_indices.end(), [&](size_t a, size_t b) {
        return m_faces[a].depth > m_faces[b].depth;
    });

    // 3. Rasterize faces with two-sided Lambertian diffuse shading
    for (size_t face_idx : sorted_indices) {
        const auto& face = m_faces[face_idx];

        const auto& p0 = rotated_vertices[face.v0];
        const auto& p1 = rotated_vertices[face.v1];
        const auto& p2 = rotated_vertices[face.v2];

        // Face normal via cross product
        const euler::vec3<float> e1 = p1 - p0;
        const euler::vec3<float> e2 = p2 - p0;
        const euler::vec3<float> cross_prod = euler::cross(e1, e2);
        const euler::vec3<float> normal = cross_prod.normalized();

        // Two-sided Lambertian diffuse reflection: |N · L|
        const float diffuse = std::abs(euler::dot(normal, light_dir));
        const float intensity = std::clamp(0.20f + 0.80f * diffuse, 0.0f, 1.0f);

        // Color index calculation based on tile type and active theme
        uint8_t color_idx = 0;
        const int shade_offset = static_cast<int>(intensity * 38.0f);

        switch (m_color_theme) {
        case 0:
            color_idx = face.is_white
                ? static_cast<uint8_t>(41 + shade_offset)  // Ivory White
                : static_cast<uint8_t>(1 + shade_offset);   // Dark Slate
            break;
        case 1:
            color_idx = face.is_white
                ? static_cast<uint8_t>(81 + shade_offset)  // Neon Cyan
                : static_cast<uint8_t>(121 + shade_offset); // Hot Magenta
            break;
        case 2:
            color_idx = face.is_white
                ? static_cast<uint8_t>(201 + shade_offset) // Brilliant Gold
                : static_cast<uint8_t>(161 + shade_offset); // Emerald Green
            break;
        default:
            break;
        }

        // Draw quad as two coplanar triangles
        const int sx0 = screen_x[face.v0];
        const int sy0 = screen_y[face.v0];
        const int sx1 = screen_x[face.v1];
        const int sy1 = screen_y[face.v1];
        const int sx2 = screen_x[face.v2];
        const int sy2 = screen_y[face.v2];
        const int sx3 = screen_x[face.v3];
        const int sy3 = screen_y[face.v3];

        rasterize_triangle(sx0, sy0, sx1, sy1, sx2, sy2, color_idx);
        rasterize_triangle(sx0, sy0, sx2, sy2, sx3, sy3, color_idx);
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
