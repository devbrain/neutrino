#include "rotate3d_effect.hh"

#include <algorithm>
#include <cmath>
#include <neutrino/video/draw.hh>

namespace demoscene {

rotate3d_effect::rotate3d_effect()
    : m_orientation(euler::quaternion<float>::identity())
{
    on_enter();
}

void rotate3d_effect::build_mesh(int model_index) {
    m_base_vertices.clear();
    m_edges.clear();
    m_faces.clear();

    if (model_index == 0) {
        // --------------------------------------------------------------------
        // Model 0: Regular Octahedron (6 vertices, 12 edges)
        // --------------------------------------------------------------------
        constexpr float s = 1.0f;
        m_base_vertices = {
            euler::vec3<float>{ s,  0,  0}, // 0: +X
            euler::vec3<float>{-s,  0,  0}, // 1: -X
            euler::vec3<float>{ 0,  s,  0}, // 2: +Y
            euler::vec3<float>{ 0, -s,  0}, // 3: -Y
            euler::vec3<float>{ 0,  0,  s}, // 4: +Z
            euler::vec3<float>{ 0,  0, -s}  // 5: -Z
        };

        m_edges = {
            {0, 2}, {2, 1}, {1, 3}, {3, 0}, // Equatorial ring
            {0, 4}, {1, 4}, {2, 4}, {3, 4}, // Top pyramid edges
            {0, 5}, {1, 5}, {2, 5}, {3, 5}  // Bottom pyramid edges
        };
    } else if (model_index == 1) {
        // --------------------------------------------------------------------
        // Model 1: Regular Icosahedron (12 vertices, 30 edges)
        // Based on Golden Ratio φ = (1 + √5)/2
        // --------------------------------------------------------------------
        const float phi = (1.0f + std::sqrt(5.0f)) * 0.5f;
        const float inv_norm = 1.0f / std::sqrt(1.0f + phi * phi);
        const float a = 1.0f * inv_norm;
        const float b = phi * inv_norm;

        m_base_vertices = {
            euler::vec3<float>{-a,  b,  0}, euler::vec3<float>{ a,  b,  0},
            euler::vec3<float>{-a, -b,  0}, euler::vec3<float>{ a, -b,  0},
            euler::vec3<float>{ 0, -a,  b}, euler::vec3<float>{ 0,  a,  b},
            euler::vec3<float>{ 0, -a, -b}, euler::vec3<float>{ 0,  a, -b},
            euler::vec3<float>{ b,  0, -a}, euler::vec3<float>{ b,  0,  a},
            euler::vec3<float>{-b,  0, -a}, euler::vec3<float>{-b,  0,  a}
        };

        m_edges = {
            {0, 11}, {0, 5}, {0, 1}, {0, 7}, {0, 10},
            {1, 5}, {5, 11}, {11, 10}, {10, 7}, {7, 1},
            {3, 9}, {3, 4}, {3, 2}, {3, 6}, {3, 8},
            {9, 4}, {4, 2}, {2, 6}, {6, 8}, {8, 9},
            {4, 5}, {4, 11}, {2, 11}, {2, 10}, {6, 10},
            {6, 7}, {8, 7}, {8, 1}, {9, 1}, {9, 5}
        };
    } else {
        // --------------------------------------------------------------------
        // Model 2: Cube / Hexahedron (8 vertices, 12 edges)
        // --------------------------------------------------------------------
        constexpr float c = 0.75f;
        m_base_vertices = {
            euler::vec3<float>{-c, -c, -c}, // 0
            euler::vec3<float>{ c, -c, -c}, // 1
            euler::vec3<float>{ c,  c, -c}, // 2
            euler::vec3<float>{-c,  c, -c}, // 3
            euler::vec3<float>{-c, -c,  c}, // 4
            euler::vec3<float>{ c, -c,  c}, // 5
            euler::vec3<float>{ c,  c,  c}, // 6
            euler::vec3<float>{-c,  c,  c}  // 7
        };

        m_edges = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Front face
            {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Back face
            {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Connecting ribs
        };
    }
}

void rotate3d_effect::on_enter() {
    build_mesh(m_current_model);
    m_orientation = euler::quaternion<float>::identity();
}

void rotate3d_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());

    // Interactive Model Switch
    if (in.pressed(sdlpp::scancode::m)) {
        m_current_model = (m_current_model + 1) % 3;
        build_mesh(m_current_model);
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
        m_zoom = 280.0f;
    }

    // Increment 3D rotation using euler::quaternion
    const float speed = m_angular_velocity.length();
    if (speed > 1e-4f) {
        const euler::vec3<float> axis = m_angular_velocity.normalized();
        const euler::radian<float> angle_delta = euler::radian<float>(speed * dt_sec);
        const euler::quaternion<float> delta_rot = euler::quaternion<float>::from_axis_angle(axis, angle_delta);

        // Quaternion multiplication rotates orientation: q_new = delta_rot * q_old
        m_orientation = (delta_rot * m_orientation).normalized();
    }
}

void rotate3d_effect::render(const neutrino::rect& viewport) {
    // Fill deep cosmos backdrop
    neutrino::draw_rect_fill(viewport, sdlpp::color{10, 12, 20, 255});

    const float center_x = static_cast<float>(viewport.x) + static_cast<float>(viewport.w) * 0.5f;
    const float center_y = static_cast<float>(viewport.y) + static_cast<float>(viewport.h) * 0.5f;
    constexpr float camera_dist_z = 3.5f;

    // 1. Rotate vertices by quaternion and project to 2D screen coordinates
    std::vector<neutrino::point> projected(m_base_vertices.size());
    std::vector<float> transformed_z(m_base_vertices.size());

    for (size_t i = 0; i < m_base_vertices.size(); ++i) {
        // Rotate vector: v' = q.rotate(v)
        const euler::vec3<float> rot_v = m_orientation.rotate(m_base_vertices[i]);
        transformed_z[i] = rot_v.z();

        // 3D Perspective Projection: x' = xc + x·D / (z + z0)
        const float z_world = rot_v.z() + camera_dist_z;
        const float inv_z = m_zoom / z_world;

        projected[i] = neutrino::point{
            static_cast<int>(center_x + rot_v.x() * inv_z),
            static_cast<int>(center_y + rot_v.y() * inv_z)
        };

        // Draw vertex glowing dots with depth-based sizing
        const int radius = std::max(2, static_cast<int>(4.0f + rot_v.z() * 1.5f));
        const uint8_t alpha = static_cast<uint8_t>(std::clamp(180.0f + rot_v.z() * 50.0f, 60.0f, 255.0f));
        neutrino::draw_circle_fill(
            projected[i].x, projected[i].y, radius,
            sdlpp::color{100, 220, 255, alpha}
        );
    }

    // 2. Render anti-aliased wireframe edges with depth shading
    for (const auto& e : m_edges) {
        const float avg_z = (transformed_z[e.a] + transformed_z[e.b]) * 0.5f;

        // Depth cueing: foreground edges are bright turquoise/cyan; background are dimmed navy
        const float depth_t = std::clamp((avg_z + 1.2f) / 2.4f, 0.0f, 1.0f);
        const uint8_t r = static_cast<uint8_t>(20 + depth_t * 60);
        const uint8_t g = static_cast<uint8_t>(80 + depth_t * 160);
        const uint8_t b = static_cast<uint8_t>(140 + depth_t * 115);
        const uint8_t a = static_cast<uint8_t>(80 + depth_t * 175);

        neutrino::draw_line_aa(projected[e.a], projected[e.b], sdlpp::color{r, g, b, a});
    }
}

} // namespace demoscene
