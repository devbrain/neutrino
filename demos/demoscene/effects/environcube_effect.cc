#include "environcube_effect.hh"

#include <algorithm>
#include <cmath>
#include <array>

namespace demoscene {

environcube_effect::environcube_effect()
    : m_envmap(256 * 256, 0)
{
    build_shape(0);
    generate_envmap(0);
    on_enter();
}

void environcube_effect::build_shape(int shape_idx) {
    m_vertices.clear();
    m_faces.clear();

    switch (shape_idx) {
    case 0: { // Chrome Cube
        constexpr float s = 45.0f;
        m_vertices = {
            {-s, -s, -s}, { s, -s, -s}, { s,  s, -s}, {-s,  s, -s},
            {-s, -s,  s}, { s, -s,  s}, { s,  s,  s}, {-s,  s,  s}
        };

        const std::array<std::array<size_t, 3>, 12> tris = {{
            {0, 2, 1}, {0, 3, 2}, // Back
            {4, 5, 6}, {4, 6, 7}, // Front
            {0, 4, 7}, {0, 7, 3}, // Left
            {1, 2, 6}, {1, 6, 5}, // Right
            {3, 7, 6}, {3, 6, 2}, // Top
            {0, 1, 5}, {0, 5, 4}  // Bottom
        }};
        for (const auto& t : tris) {
            m_faces.push_back({t[0], t[1], t[2], 0.0f});
        }
        break;
    }
    case 1: { // Diamond Octahedron
        constexpr float s = 65.0f;
        m_vertices = {
            { 0, -s,  0}, { s,  0,  0}, { 0,  0,  s},
            {-s,  0,  0}, { 0,  0, -s}, { 0,  s,  0}
        };

        const std::array<std::array<size_t, 3>, 8> tris = {{
            {0, 2, 1}, {0, 3, 2}, {0, 4, 3}, {0, 1, 4},
            {5, 1, 2}, {5, 2, 3}, {5, 3, 4}, {5, 4, 1}
        }};
        for (const auto& t : tris) {
            m_faces.push_back({t[0], t[1], t[2], 0.0f});
        }
        break;
    }
    case 2: { // Hexagonal Torus Ring
        m_vertices = {
            {-25, -25,  35}, { 25, -25,  35}, { 50, -50,   0}, { 25, -25, -35},
            {-25, -25, -35}, {-50, -50,   0}, {-25,  25,  35}, { 25,  25,  35},
            { 50,  50,   0}, { 25,  25, -35}, {-25,  25, -35}, {-50,  50,   0}
        };

        const std::array<std::array<size_t, 3>, 16> tris = {{
            {0, 7, 1}, {0, 6, 7}, {1, 8, 2}, {1, 7, 8},
            {2, 9, 3}, {2, 8, 9}, {3, 10, 4}, {3, 9, 10},
            {4, 11, 5}, {4, 10, 11}, {5, 6, 0}, {5, 11, 6},
            {0, 2, 1}, {2, 4, 3}, {6, 7, 8}, {8, 10, 9}
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

void environcube_effect::generate_envmap(int theme_idx) {
    constexpr int size = 256;
    constexpr float half_size = 128.0f;

    for (int y = 0; y < size; ++y) {
        const float ny = (static_cast<float>(y) - half_size) / half_size;
        for (int x = 0; x < size; ++x) {
            const float nx = (static_cast<float>(x) - half_size) / half_size;
            const float r2 = nx * nx + ny * ny;

            uint8_t col = 0;
            if (r2 <= 1.0f) {
                const float nz = std::sqrt(1.0f - r2);

                switch (theme_idx) {
                case 0: { // Studio Chrome Sphere (dual softbox lights)
                    const float s1 = std::pow(std::max(0.0f, nx * 0.5f + ny * 0.7f + nz * 0.5f), 12.0f);
                    const float s2 = std::pow(std::max(0.0f, -nx * 0.6f - ny * 0.4f + nz * 0.7f), 8.0f);
                    const float base = nz * 0.45f + 0.10f;
                    col = static_cast<uint8_t>(std::clamp((base + s1 * 0.65f + s2 * 0.35f) * 254.0f + 1.0f, 1.0f, 255.0f));
                    break;
                }
                case 1: { // Sunset Horizon
                    const float horizon = std::sin(ny * 3.14159f * 0.5f);
                    const float sun = std::pow(std::max(0.0f, nx * 0.2f + ny * 0.2f + nz * 0.95f), 16.0f);
                    const float val = std::abs(horizon) * 0.5f + sun * 0.5f;
                    col = static_cast<uint8_t>(std::clamp(val * 254.0f + 1.0f, 1.0f, 255.0f));
                    break;
                }
                case 2: { // Cyber Matrix Grid Reflection
                    const int gx = static_cast<int>(std::abs(nx) * 12.0f);
                    const int gy = static_cast<int>(std::abs(ny) * 12.0f);
                    const bool grid = (gx % 2 == 0) || (gy % 2 == 0);
                    const float val = grid ? (0.7f + 0.3f * nz) : (0.2f * nz);
                    col = static_cast<uint8_t>(std::clamp(val * 254.0f + 1.0f, 1.0f, 255.0f));
                    break;
                }
                default:
                    break;
                }
            }
            m_envmap[y * size + x] = col;
        }
    }
}

void environcube_effect::init_palette() {
    m_canvas.set_rgb(0, 8, 10, 16); // Deep space backdrop

    switch (m_theme_idx) {
    case 0: { // Liquid Chrome: Deep metallic steel -> Bright silver -> Blinding specular white
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(30 + u * 150),
                    static_cast<uint8_t>(40 + u * 165),
                    static_cast<uint8_t>(60 + u * 195));
            } else {
                const float u = (t - 0.6f) / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(180 + u * 75),
                    static_cast<uint8_t>(205 + u * 50),
                    255);
            }
        }
        break;
    }
    case 1: { // Sunset Horizon: Deep violet -> Amber flame -> Radiant gold
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.5f) {
                const float u = t / 0.5f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(60 + u * 180),
                    static_cast<uint8_t>(10 + u * 60),
                    static_cast<uint8_t>(80 - u * 60));
            } else {
                const float u = (t - 0.5f) / 0.5f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    240,
                    static_cast<uint8_t>(70 + u * 180),
                    static_cast<uint8_t>(20 + u * 180));
            }
        }
        break;
    }
    case 2: { // Cyber Matrix: Midnight black -> Toxic lime green -> Electric cyan
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(u * 30),
                    static_cast<uint8_t>(30 + u * 200),
                    static_cast<uint8_t>(20 + u * 40));
            } else {
                const float u = (t - 0.6f) / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(30 + u * 180),
                    230 + static_cast<uint8_t>(u * 25),
                    static_cast<uint8_t>(60 + u * 195));
            }
        }
        break;
    }
    default:
        break;
    }
}

void environcube_effect::on_enter() {
    init_palette();
    m_canvas.clear(0);
    m_orientation = euler::quaternion<float>::identity();
    m_angular_velocity = euler::vec3<float>{0.5f, 0.8f, 0.3f};
    m_zoom = 240.0f;
    m_time = 0.0f;
    m_shape_idx = 0;
    m_theme_idx = 0;
    build_shape(0);
    generate_envmap(0);
}

void environcube_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    if (in.pressed(sdlpp::scancode::space)) {
        m_shape_idx = (m_shape_idx + 1) % 3;
        build_shape(m_shape_idx);
    }

    if (in.pressed(sdlpp::scancode::c)) {
        m_theme_idx = (m_theme_idx + 1) % 3;
        generate_envmap(m_theme_idx);
        init_palette();
    }

    if (in.held(sdlpp::scancode::w)) m_zoom = std::min(450.0f, m_zoom + 100.0f * dt_sec);
    if (in.held(sdlpp::scancode::s)) m_zoom = std::max(120.0f, m_zoom - 100.0f * dt_sec);

    if (in.held(sdlpp::scancode::left))  m_angular_velocity[1] -= 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::right)) m_angular_velocity[1] += 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::up))    m_angular_velocity[0] -= 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::down))  m_angular_velocity[0] += 2.0f * dt_sec;

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }

    const float speed = m_angular_velocity.length();
    if (speed > 1e-4f) {
        const euler::vec3<float> axis = m_angular_velocity.normalized();
        const euler::radian<float> angle_delta = euler::radian<float>(speed * dt_sec);
        const euler::quaternion<float> delta_rot = euler::quaternion<float>::from_axis_angle(axis, angle_delta);
        m_orientation = (delta_rot * m_orientation).normalized();
    }
}

void environcube_effect::rasterize_env_triangle(int x0, int y0, float u0, float v0,
                                                int x1, int y1, float u1, float v1,
                                                int x2, int y2, float u2, float v2) {
    if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); std::swap(u0, u1); std::swap(v0, v1); }
    if (y0 > y2) { std::swap(x0, x2); std::swap(y0, y2); std::swap(u0, u2); std::swap(v0, v2); }
    if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); std::swap(u1, u2); std::swap(v1, v2); }

    if (y0 == y2 || y2 < 0 || y0 >= vga_canvas::height) return;

    const int total_height = y2 - y0;
    uint8_t* raw = m_canvas.raw_pixels();
    const uint8_t* env = m_envmap.data();

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

        float ua = u0 + (u2 - u0) * alpha;
        float va = v0 + (v2 - v0) * alpha;
        float ub = second_half ? (u1 + (u2 - u1) * beta) : (u0 + (u1 - u0) * beta);
        float vb = second_half ? (v1 + (v2 - v1) * beta) : (v0 + (v1 - v0) * beta);

        if (xa > xb) {
            std::swap(xa, xb);
            std::swap(ua, ub);
            std::swap(va, vb);
        }

        const int cl_start = std::max(0, xa);
        const int cl_end = std::min(vga_canvas::width - 1, xb);
        const float span = static_cast<float>(xb - xa);

        uint8_t* row = raw + y * vga_canvas::width;

        for (int x = cl_start; x <= cl_end; ++x) {
            const float t = (span > 0.0f) ? (static_cast<float>(x - xa) / span) : 0.0f;
            const float cur_u = ua + (ub - ua) * t;
            const float cur_v = va + (vb - va) * t;

            const int iu = std::clamp(static_cast<int>(cur_u), 0, 255);
            const int iv = std::clamp(static_cast<int>(cur_v), 0, 255);

            row[x] = env[(iv << 8) | iu];
        }
    }
}

void environcube_effect::render(const neutrino::rect& viewport) {
    m_canvas.clear(0);

    constexpr float camera_z = 220.0f;

    // 1. Rotate all vertices and compute screen coordinates
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

    // 2. Compute face depth for Painter's algorithm
    for (auto& face : m_faces) {
        face.depth = (rotated_v[face.v0].z() + rotated_v[face.v1].z() + rotated_v[face.v2].z()) / 3.0f;
    }

    std::vector<size_t> sorted_indices(m_faces.size());
    for (size_t i = 0; i < sorted_indices.size(); ++i) sorted_indices[i] = i;

    std::sort(sorted_indices.begin(), sorted_indices.end(), [&](size_t a, size_t b) {
        return m_faces[a].depth > m_faces[b].depth;
    });

    // 3. Rasterize environment-mapped triangles
    for (size_t face_idx : sorted_indices) {
        const auto& face = m_faces[face_idx];

        const auto& p0 = rotated_v[face.v0];
        const auto& p1 = rotated_v[face.v1];
        const auto& p2 = rotated_v[face.v2];

        // Face normal in view space
        const euler::vec3<float> e1 = p1 - p0;
        const euler::vec3<float> e2 = p2 - p0;
        const euler::vec3<float> cross_prod = euler::cross(e1, e2);
        const float norm_len = cross_prod.length();
        if (norm_len < 1e-4f) continue;
        const euler::vec3<float> normal = cross_prod / norm_len;

        // Backface culling: only front-facing faces (normal pointing toward viewer z < 0 or > 0)
        // Camera looks down -Z axis
        if (normal.z() >= 0.0f) continue;

        // Spherical reflection vector: R = 2(N·V)N - V where V = (0, 0, -1)
        const float dot_nv = -normal.z();
        const float rx = 2.0f * dot_nv * normal.x();
        const float ry = 2.0f * dot_nv * normal.y();

        // Environment map UV coordinates
        const float u = rx * 115.0f + 128.0f;
        const float v = ry * 115.0f + 128.0f;

        rasterize_env_triangle(
            screen_x[face.v0], screen_y[face.v0], u, v,
            screen_x[face.v1], screen_y[face.v1], u, v,
            screen_x[face.v2], screen_y[face.v2], u, v
        );
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
