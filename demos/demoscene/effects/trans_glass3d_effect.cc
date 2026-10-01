#include "trans_glass3d_effect.hh"

#include <algorithm>
#include <cmath>

namespace demoscene {

trans_glass3d_effect::trans_glass3d_effect() {
    build_shape(0);
    on_enter();
}

void trans_glass3d_effect::build_shape(int shape_idx) {
    m_vertices.clear();
    m_faces.clear();

    switch (shape_idx) {
    case 0: {
        // Glass Cube (8 vertices, 6 quads = 12 triangles)
        constexpr float s = 45.0f;
        m_vertices = {
            {-s, -s, -s}, { s, -s, -s}, { s,  s, -s}, {-s,  s, -s},
            {-s, -s,  s}, { s, -s,  s}, { s,  s,  s}, {-s,  s,  s}
        };

        // 12 triangles
        const std::array<std::array<size_t, 3>, 12> tris = {{
            {0, 1, 2}, {0, 2, 3}, // Back
            {5, 4, 7}, {5, 7, 6}, // Front
            {4, 0, 3}, {4, 3, 7}, // Left
            {1, 5, 6}, {1, 6, 2}, // Right
            {3, 2, 6}, {3, 6, 7}, // Top
            {4, 5, 1}, {4, 1, 0}  // Bottom
        }};
        for (const auto& t : tris) {
            m_faces.push_back({t[0], t[1], t[2], 0.0f});
        }
        break;
    }
    case 1: {
        // Diamond Octahedron (6 vertices, 8 triangles)
        constexpr float s = 65.0f;
        m_vertices = {
            { 0, -s,  0}, { s,  0,  0}, { 0,  0,  s},
            {-s,  0,  0}, { 0,  0, -s}, { 0,  s,  0}
        };

        const std::array<std::array<size_t, 3>, 8> tris = {{
            {0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 1},
            {5, 2, 1}, {5, 3, 2}, {5, 4, 3}, {5, 1, 4}
        }};
        for (const auto& t : tris) {
            m_faces.push_back({t[0], t[1], t[2], 0.0f});
        }
        break;
    }
    case 2: {
        // Hollow Hexagonal Torus ring (3D_HOLE.PAS: 12 vertices, 16 triangles)
        m_vertices = {
            {-25, -25,  35}, { 25, -25,  35}, { 50, -50,   0}, { 25, -25, -35},
            {-25, -25, -35}, {-50, -50,   0}, {-25,  25,  35}, { 25,  25,  35},
            { 50,  50,   0}, { 25,  25, -35}, {-25,  25, -35}, {-50,  50,   0}
        };

        const std::array<std::array<size_t, 3>, 16> tris = {{
            {0, 1, 7}, {0, 7, 6}, {1, 2, 8}, {1, 8, 7},
            {2, 3, 9}, {2, 9, 8}, {3, 4, 10}, {3, 10, 9},
            {4, 5, 11}, {4, 11, 10}, {5, 0, 6}, {5, 6, 11},
            {0, 1, 2}, {2, 3, 4}, {6, 7, 8}, {8, 9, 10}
        }};
        for (const auto& t : tris) {
            m_faces.push_back({t[0], t[1], t[2], 0.0f});
        }
        break;
    }
    default:
        break;
    }
}

void trans_glass3d_effect::init_palette() {
    m_canvas.set_rgb(0, 6, 8, 16); // Dark space background

    switch (m_tint_theme) {
    case 0: {
        // Emerald Cyan Glass:
        // Deep teal -> Vivid cyan -> Incandescent ice white
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            uint8_t r = 0, g = 0, b = 0;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                r = static_cast<uint8_t>(u * 30);
                g = static_cast<uint8_t>(40 + u * 175);
                b = static_cast<uint8_t>(60 + u * 175);
            } else {
                const float u = (t - 0.6f) / 0.4f;
                r = static_cast<uint8_t>(30 + u * 225);
                g = static_cast<uint8_t>(215 + u * 40);
                b = 255;
            }
            m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
        }
        break;
    }
    case 1: {
        // Amethyst Ruby Glass:
        // Deep violet -> Vibrant ruby magenta -> Blinding pink-white
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            uint8_t r = 0, g = 0, b = 0;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                r = static_cast<uint8_t>(60 + u * 175);
                g = static_cast<uint8_t>(u * 30);
                b = static_cast<uint8_t>(50 + u * 120);
            } else {
                const float u = (t - 0.6f) / 0.4f;
                r = 255;
                g = static_cast<uint8_t>(30 + u * 225);
                b = static_cast<uint8_t>(170 + u * 85);
            }
            m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
        }
        break;
    }
    case 2: {
        // Amber Sun Glass:
        // Deep mahogany -> Rich golden amber -> Solar white
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            uint8_t r = 0, g = 0, b = 0;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                r = static_cast<uint8_t>(70 + u * 175);
                g = static_cast<uint8_t>(20 + u * 150);
                b = static_cast<uint8_t>(u * 20);
            } else {
                const float u = (t - 0.6f) / 0.4f;
                r = 255;
                g = static_cast<uint8_t>(170 + u * 85);
                b = static_cast<uint8_t>(20 + u * 235);
            }
            m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
        }
        break;
    }
    default:
        break;
    }
}

void trans_glass3d_effect::on_enter() {
    init_palette();
    m_orientation = euler::quaternion<float>::identity();
    m_angular_velocity = euler::vec3<float>{0.5f, 0.8f, 0.3f};
    m_zoom = 240.0f;
    m_time = 0.0f;
    m_shape_mode = 0;
    m_tint_theme = 0;
    build_shape(0);
}

void trans_glass3d_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    // Switch 3D Glass Geometry
    if (in.pressed(sdlpp::scancode::space)) {
        m_shape_mode = (m_shape_mode + 1) % 3;
        build_shape(m_shape_mode);
    }

    // Switch Glass Color Tint Theme
    if (in.pressed(sdlpp::scancode::c)) {
        m_tint_theme = (m_tint_theme + 1) % 3;
        init_palette();
    }

    // Zoom Controls
    if (in.held(sdlpp::scancode::w)) m_zoom = std::min(450.0f, m_zoom + 100.0f * dt_sec);
    if (in.held(sdlpp::scancode::s)) m_zoom = std::max(120.0f, m_zoom - 100.0f * dt_sec);

    // Interactive Manual Rotation Bias
    if (in.held(sdlpp::scancode::left))  m_angular_velocity[1] -= 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::right)) m_angular_velocity[1] += 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::up))    m_angular_velocity[0] -= 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::down))  m_angular_velocity[0] += 2.0f * dt_sec;

    if (in.pressed(sdlpp::scancode::r)) {
        m_orientation = euler::quaternion<float>::identity();
        m_angular_velocity = euler::vec3<float>{0.5f, 0.8f, 0.3f};
        m_zoom = 240.0f;
        m_shape_mode = 0;
        m_tint_theme = 0;
        build_shape(0);
        init_palette();
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

void trans_glass3d_effect::rasterize_glass_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t tint_val) {
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

        // Additive translucent blending: mem[es:di] += tint_val
        for (int x = cl_start; x <= cl_end; ++x) {
            const uint8_t cur = m_canvas.get_pixel(x, y);
            const uint8_t add_val = static_cast<uint8_t>(std::min(255, static_cast<int>(cur) + static_cast<int>(tint_val)));
            m_canvas.put_pixel_fast(x, y, add_val);
        }
    }
}

void trans_glass3d_effect::render(const neutrino::rect& viewport) {
    m_canvas.clear(0);

    constexpr float camera_z = 220.0f;
    const euler::vec3<float> light_dir = euler::vec3<float>{0.4f, 0.7f, -1.0f}.normalized();

    // 1. Rotate all vertices with quaternion
    std::vector<euler::vec3<float>> rotated_v;
    rotated_v.reserve(m_vertices.size());

    std::vector<int> screen_x(m_vertices.size());
    std::vector<int> screen_y(m_vertices.size());

    for (size_t i = 0; i < m_vertices.size(); ++i) {
        const euler::vec3<float> rot = m_orientation.rotate(m_vertices[i]);
        rotated_v.push_back(rot);

        const float pz = rot.z() + camera_z;
        const float inv_z = (pz > 1.0f) ? (m_zoom / pz) : 1.0f;

        screen_x[i] = 160 + static_cast<int>(rot.x() * inv_z);
        screen_y[i] = 100 + static_cast<int>(rot.y() * inv_z);
    }

    // 2. Compute face depth for Painter's Algorithm sorting
    for (auto& face : m_faces) {
        face.depth = (rotated_v[face.v0].z() + rotated_v[face.v1].z() + rotated_v[face.v2].z()) / 3.0f;
    }

    // Sort back to front (largest depth first)
    std::vector<size_t> sorted_indices(m_faces.size());
    for (size_t i = 0; i < sorted_indices.size(); ++i) sorted_indices[i] = i;

    std::sort(sorted_indices.begin(), sorted_indices.end(), [&](size_t a, size_t b) {
        return m_faces[a].depth > m_faces[b].depth;
    });

    // 3. Rasterize translucent faces with additive glass blending
    for (size_t face_idx : sorted_indices) {
        const auto& face = m_faces[face_idx];

        const auto& p0 = rotated_v[face.v0];
        const auto& p1 = rotated_v[face.v1];
        const auto& p2 = rotated_v[face.v2];

        // Face normal via cross product
        const euler::vec3<float> e1 = p1 - p0;
        const euler::vec3<float> e2 = p2 - p0;
        const euler::vec3<float> cross_prod = euler::cross(e1, e2);
        const euler::vec3<float> normal = cross_prod.normalized();

        // Two-sided Fresnel / diffuse reflection for glass
        const float diffuse = std::abs(euler::dot(normal, light_dir));
        const float fresnel = 1.0f - std::abs(normal.z()); // Edge brightening (Fresnel effect)
        const float intensity = std::clamp(0.25f * diffuse + 0.75f * fresnel, 0.0f, 1.0f);

        // Glass tint intensity (40..90 per face) so multiple overlapping faces add to full saturation!
        const uint8_t tint = static_cast<uint8_t>(45.0f + intensity * 45.0f);

        const int sx0 = screen_x[face.v0];
        const int sy0 = screen_y[face.v0];
        const int sx1 = screen_x[face.v1];
        const int sy1 = screen_y[face.v1];
        const int sx2 = screen_x[face.v2];
        const int sy2 = screen_y[face.v2];

        rasterize_glass_triangle(sx0, sy0, sx1, sy1, sx2, sy2, tint);
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
