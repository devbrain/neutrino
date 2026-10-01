#include "texture_cube_effect.hh"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace demoscene {

texture_cube_effect::texture_cube_effect() {
    init_palette();
    generate_textures();

    // Build cube with 24 vertices (4 per face) for precise UV mapping
    constexpr float s = 40.0f;
    constexpr float uv_max = 63.0f;

    auto add_face = [this](
        const euler::vec3<float>& p0, const euler::vec3<float>& p1,
        const euler::vec3<float>& p2, const euler::vec3<float>& p3,
        const euler::vec3<float>& normal) {
        const int base = static_cast<int>(m_vertices.size());
        m_vertices.push_back({p0, 0.0f, 0.0f});
        m_vertices.push_back({p1, uv_max, 0.0f});
        m_vertices.push_back({p2, uv_max, uv_max});
        m_vertices.push_back({p3, 0.0f, uv_max});

        m_triangles.push_back({base + 0, base + 1, base + 2, normal});
        m_triangles.push_back({base + 0, base + 2, base + 3, normal});
    };

    // Front (+Z)
    add_face({-s, -s,  s}, { s, -s,  s}, { s,  s,  s}, {-s,  s,  s}, { 0,  0,  1});
    // Back (-Z)
    add_face({ s, -s, -s}, {-s, -s, -s}, {-s,  s, -s}, { s,  s, -s}, { 0,  0, -1});
    // Right (+X)
    add_face({ s, -s,  s}, { s, -s, -s}, { s,  s, -s}, { s,  s,  s}, { 1,  0,  0});
    // Left (-X)
    add_face({-s, -s, -s}, {-s, -s,  s}, {-s,  s,  s}, {-s,  s, -s}, {-1,  0,  0});
    // Top (+Y)
    add_face({-s,  s,  s}, { s,  s,  s}, { s,  s, -s}, {-s,  s, -s}, { 0,  1,  0});
    // Bottom (-Y)
    add_face({-s, -s, -s}, { s, -s, -s}, { s, -s,  s}, {-s, -s,  s}, { 0, -1,  0});

    on_enter();
}

void texture_cube_effect::init_palette() {
    m_canvas.set_rgb(0, 0, 0, 0); // Black

    // Palette ramp partitioned into 4 lighting banks of 64 colors
    // Bank 0 (1..63): Gold / Amber Gradient for Texture 0
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        const uint8_t r = static_cast<uint8_t>(std::min(255.0f, t * 290.0f));
        const uint8_t g = static_cast<uint8_t>(t * 220.0f);
        const uint8_t b = static_cast<uint8_t>(t * t * 120.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
    }

    // Bank 1 (64..127): Cyber Cyan Neon Gradient for Texture 1
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        const uint8_t r = static_cast<uint8_t>(t * t * 80.0f);
        const uint8_t g = static_cast<uint8_t>(40.0f + t * 215.0f);
        const uint8_t b = static_cast<uint8_t>(100.0f + t * 155.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(64 + i), r, g, b);
    }

    // Bank 2 (128..191): Ruby / Magenta Gradient for Texture 2
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        const uint8_t r = static_cast<uint8_t>(60.0f + t * 195.0f);
        const uint8_t g = static_cast<uint8_t>(t * t * 90.0f);
        const uint8_t b = static_cast<uint8_t>(t * 180.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(128 + i), r, g, b);
    }

    // Bank 3 (192..255): Monochrome Chrome Gradient
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        const uint8_t val = static_cast<uint8_t>(t * 255.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(192 + i), val, val, val);
    }
}

void texture_cube_effect::generate_textures() {
    // ------------------------------------------------------------------------
    // Texture 0: Checkerboard with Gold Bevel Frame
    // ------------------------------------------------------------------------
    for (int y = 0; y < tex_size; ++y) {
        for (int x = 0; x < tex_size; ++x) {
            const bool border = (x < 3 || x >= tex_size - 3 || y < 3 || y >= tex_size - 3);
            const bool check = (((x / 8) ^ (y / 8)) & 1) != 0;
            const uint8_t val = border ? 58 : (check ? 48 : 18);
            m_textures[0][y * tex_size + x] = val;
        }
    }

    // ------------------------------------------------------------------------
    // Texture 1: Cyber Grid with Circuit Traces
    // ------------------------------------------------------------------------
    for (int y = 0; y < tex_size; ++y) {
        for (int x = 0; x < tex_size; ++x) {
            const bool grid = (x % 16 == 0 || y % 16 == 0);
            const bool center_chip = (x >= 24 && x < 40 && y >= 24 && y < 40);
            uint8_t val = 70; // Dark cyber blue
            if (grid) val = 110; // Bright trace
            if (center_chip) val = 125; // Core chip
            m_textures[1][y * tex_size + x] = val;
        }
    }

    // ------------------------------------------------------------------------
    // Texture 2: Concentric Target Mandala
    // ------------------------------------------------------------------------
    for (int y = 0; y < tex_size; ++y) {
        const float dy = static_cast<float>(y - 32);
        for (int x = 0; x < tex_size; ++x) {
            const float dx = static_cast<float>(x - 32);
            const float r = std::sqrt(dx * dx + dy * dy);
            const int ring = static_cast<int>(r / 5.0f) % 2;
            const uint8_t val = (ring == 0) ? 180 : 140;
            m_textures[2][y * tex_size + x] = val;
        }
    }
}

void texture_cube_effect::on_enter() {
    m_canvas.clear(0);
    m_orientation = euler::quaternion<float>::identity();
    m_angular_velocity = {0.7f, 1.1f, 0.45f};
    m_zoom = 150.0f;
    m_time = 0.0f;
}

void texture_cube_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    if (in.pressed(sdlpp::scancode::space)) {
        m_texture_idx = (m_texture_idx + 1) % num_textures;
    }

    if (in.pressed(sdlpp::scancode::l)) {
        m_enable_lighting = !m_enable_lighting;
    }

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }

    if (in.held(sdlpp::scancode::w)) m_zoom = std::min(300.0f, m_zoom + 80.0f * dt_sec);
    if (in.held(sdlpp::scancode::s)) m_zoom = std::max(60.0f, m_zoom - 80.0f * dt_sec);

    if (in.held(sdlpp::scancode::left))  m_angular_velocity[1] -= 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::right)) m_angular_velocity[1] += 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::up))    m_angular_velocity[0] -= 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::down))  m_angular_velocity[0] += 2.0f * dt_sec;

    // Quaternion auto-rotation
    const float speed = m_angular_velocity.length();
    if (speed > 1e-4f) {
        const euler::vec3<float> axis = m_angular_velocity.normalized();
        const euler::radian<float> angle_delta = euler::radian<float>(speed * dt_sec);
        const euler::quaternion<float> delta_rot = euler::quaternion<float>::from_axis_angle(axis, angle_delta);
        m_orientation = (delta_rot * m_orientation).normalized();
    }
}

void texture_cube_effect::draw_textured_triangle(
    float x0, float y0, float u0, float v0,
    float x1, float y1, float u1, float v1,
    float x2, float y2, float u2, float v2,
    float light_intensity, uint8_t* raw)
{
    // Sort vertices by Y: p0.y <= p1.y <= p2.y
    if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); std::swap(u0, u1); std::swap(v0, v1); }
    if (y0 > y2) { std::swap(x0, x2); std::swap(y0, y2); std::swap(u0, u2); std::swap(v0, v2); }
    if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); std::swap(u1, u2); std::swap(v1, v2); }

    const int iy0 = static_cast<int>(std::ceil(y0));
    const int iy2 = static_cast<int>(std::floor(y2));
    if (iy0 > iy2) return;

    const float total_height = y2 - y0;
    if (total_height <= 0.001f) return;

    const uint8_t* tex = m_textures[m_texture_idx].data();
    const float light_mult = m_enable_lighting ? light_intensity : 1.0f;

    for (int y = std::max(0, iy0); y <= std::min(vga_canvas::height - 1, iy2); ++y) {
        const bool second_half = (static_cast<float>(y) > y1) || (y1 == y0);
        const float segment_height = second_half ? (y2 - y1) : (y1 - y0);

        if (segment_height <= 0.001f) continue;

        const float alpha = (static_cast<float>(y) - y0) / total_height;
        const float beta = second_half
            ? (static_cast<float>(y) - y1) / (y2 - y1)
            : (static_cast<float>(y) - y0) / (y1 - y0);

        float xa = x0 + (x2 - x0) * alpha;
        float ua = u0 + (u2 - u0) * alpha;
        float va = v0 + (v2 - v0) * alpha;

        float xb = second_half ? (x1 + (x2 - x1) * beta) : (x0 + (x1 - x0) * beta);
        float ub = second_half ? (u1 + (u2 - u1) * beta) : (u0 + (u1 - u0) * beta);
        float vb = second_half ? (v1 + (v2 - v1) * beta) : (v0 + (v1 - v0) * beta);

        if (xa > xb) {
            std::swap(xa, xb);
            std::swap(ua, ub);
            std::swap(va, vb);
        }

        const int ixa = static_cast<int>(std::ceil(xa));
        const int ixb = static_cast<int>(std::floor(xb));
        const float span_width = xb - xa;

        if (span_width > 0.001f) {
            for (int x = std::max(0, ixa); x <= std::min(vga_canvas::width - 1, ixb); ++x) {
                const float t = (static_cast<float>(x) - xa) / span_width;
                const int u = static_cast<int>(ua + (ub - ua) * t) & 63;
                const int v = static_cast<int>(va + (vb - va) * t) & 63;

                const uint8_t tex_col = tex[v * tex_size + u];
                const uint8_t shaded_col = static_cast<uint8_t>(std::clamp(static_cast<int>(static_cast<float>(tex_col) * light_mult), 1, 255));
                raw[y * vga_canvas::width + x] = shaded_col;
            }
        }
    }
}

void texture_cube_effect::render(const neutrino::rect& viewport) {
    m_canvas.clear(0);
    uint8_t* raw = m_canvas.raw_pixels();

    constexpr float camera_z = 210.0f;

    const auto mat = m_orientation.to_matrix3();
    const float r00 = mat(0, 0), r01 = mat(0, 1), r02 = mat(0, 2);
    const float r10 = mat(1, 0), r11 = mat(1, 1), r12 = mat(1, 2);
    const float r20 = mat(2, 0), r21 = mat(2, 1), r22 = mat(2, 2);

    // Transform vertices to screen coordinates
    struct TransformedVertex {
        float sx, sy;
        float u, v;
    };
    std::vector<TransformedVertex> tv(m_vertices.size());

    for (size_t i = 0; i < m_vertices.size(); ++i) {
        const auto& pt = m_vertices[i].pos;
        const float x = pt.x();
        const float y = pt.y();
        const float z = pt.z();

        const float pz = r20 * x + r21 * y + r22 * z + camera_z;
        const float inv_z = (pz > 10.0f) ? (m_zoom / pz) : 1.0f;

        tv[i].sx = 160.0f + (r00 * x + r01 * y + r02 * z) * inv_z;
        tv[i].sy = 100.0f + (r10 * x + r11 * y + r12 * z) * inv_z;
        tv[i].u = m_vertices[i].u;
        tv[i].v = m_vertices[i].v;
    }

    // Directional light vector normalized (top-right-front)
    constexpr float lx = 0.577f;
    constexpr float ly = 0.577f;
    constexpr float lz = 0.577f;

    // Render triangles with backface culling
    for (const auto& tri : m_triangles) {
        const auto& p0 = tv[static_cast<size_t>(tri.v0)];
        const auto& p1 = tv[static_cast<size_t>(tri.v1)];
        const auto& p2 = tv[static_cast<size_t>(tri.v2)];

        // Backface culling: 2D screen cross product
        const float cross = (p1.sx - p0.sx) * (p2.sy - p0.sy) - (p1.sy - p0.sy) * (p2.sx - p0.sx);
        if (cross <= 0.0f) continue; // Face points away from viewer

        // Rotated face normal
        const auto& norm = tri.normal;
        const float nx = r00 * norm.x() + r01 * norm.y() + r02 * norm.z();
        const float ny = r10 * norm.x() + r11 * norm.y() + r12 * norm.z();
        const float nz = r20 * norm.x() + r21 * norm.y() + r22 * norm.z();

        // Lambertian lighting: ambient 0.35 + diffuse 0.65
        const float dot = std::max(0.0f, nx * lx + ny * ly + nz * lz);
        const float light_intensity = 0.35f + 0.65f * dot;

        draw_textured_triangle(
            p0.sx, p0.sy, p0.u, p0.v,
            p1.sx, p1.sy, p1.u, p1.v,
            p2.sx, p2.sy, p2.u, p2.v,
            light_intensity, raw);
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
