#include "glenz_vector_effect.hh"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace demoscene {

glenz_vector_effect::glenz_vector_effect() {
    build_models();
    init_palette();
    on_enter();
}

void glenz_vector_effect::build_models() {
    m_models.resize(static_cast<size_t>(PolyType::Count));

    // ------------------------------------------------------------------------
    // Model 0: Glenz Cube (8 vertices, 6 quad faces)
    // ------------------------------------------------------------------------
    {
        auto& model = m_models[static_cast<size_t>(PolyType::Cube)];
        constexpr float s = 45.0f;
        model.vertices = {
            {-s, -s, -s}, { s, -s, -s}, { s,  s, -s}, {-s,  s, -s},
            {-s, -s,  s}, { s, -s,  s}, { s,  s,  s}, {-s,  s,  s}
        };
        model.faces = {
            {{0, 1, 2, 3}, 45}, // Front
            {{5, 4, 7, 6}, 45}, // Back
            {{4, 0, 3, 7}, 55}, // Left
            {{1, 5, 6, 2}, 55}, // Right
            {{4, 5, 1, 0}, 65}, // Top
            {{3, 2, 6, 7}, 65}  // Bottom
        };
    }

    // ------------------------------------------------------------------------
    // Model 1: Diamond Octahedron (6 vertices, 8 triangular faces)
    // ------------------------------------------------------------------------
    {
        auto& model = m_models[static_cast<size_t>(PolyType::Octahedron)];
        constexpr float r = 60.0f;
        model.vertices = {
            { 0,  r,  0}, // Top
            { 0, -r,  0}, // Bottom
            { r,  0,  0}, // Right
            {-r,  0,  0}, // Left
            { 0,  0,  r}, // Front
            { 0,  0, -r}  // Back
        };
        model.faces = {
            {{0, 2, 4}, 50}, {{0, 4, 3}, 55}, {{0, 3, 5}, 50}, {{0, 5, 2}, 55},
            {{1, 4, 2}, 60}, {{1, 3, 4}, 65}, {{1, 5, 3}, 60}, {{1, 2, 5}, 65}
        };
    }

    // ------------------------------------------------------------------------
    // Model 2: Stella Octangula / Star Polyhedron (compound tetrahedra)
    // ------------------------------------------------------------------------
    {
        auto& model = m_models[static_cast<size_t>(PolyType::StarOctangula)];
        constexpr float s = 32.0f;
        constexpr float tip = 64.0f;
        // Inner cube + 6 pyramid tips
        model.vertices = {
            {-s, -s, -s}, { s, -s, -s}, { s,  s, -s}, {-s,  s, -s},
            {-s, -s,  s}, { s, -s,  s}, { s,  s,  s}, {-s,  s,  s},
            { tip, 0, 0}, {-tip, 0, 0}, {0,  tip, 0}, {0, -tip, 0}, {0, 0,  tip}, {0, 0, -tip}
        };
        // 24 triangular star facets
        model.faces = {
            {{8, 1, 2}, 40}, {{8, 2, 6}, 45}, {{8, 6, 5}, 40}, {{8, 5, 1}, 45},
            {{9, 0, 4}, 40}, {{9, 4, 7}, 45}, {{9, 7, 3}, 40}, {{9, 3, 0}, 45},
            {{10, 3, 2}, 50}, {{10, 2, 6}, 55}, {{10, 6, 7}, 50}, {{10, 7, 3}, 55},
            {{11, 0, 1}, 50}, {{11, 1, 5}, 55}, {{11, 5, 4}, 50}, {{11, 4, 0}, 55},
            {{12, 4, 5}, 60}, {{12, 5, 6}, 65}, {{12, 6, 7}, 60}, {{12, 7, 4}, 65},
            {{13, 0, 3}, 60}, {{13, 3, 2}, 65}, {{13, 2, 1}, 60}, {{13, 1, 0}, 65}
        };
    }
}

void glenz_vector_effect::init_palette() {
    m_canvas.set_rgb(0, 0, 0, 0); // Black

    switch (m_theme) {
    case 0: { // Ruby Crystal Flame
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            const uint8_t r = static_cast<uint8_t>(std::min(255.0f, t * 300.0f));
            const uint8_t g = static_cast<uint8_t>(t * t * 180.0f);
            const uint8_t b = static_cast<uint8_t>(t * t * t * 240.0f);
            m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
        }
        break;
    }
    case 1: { // Emerald Matrix Gem
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            const uint8_t r = static_cast<uint8_t>(t * t * 140.0f);
            const uint8_t g = static_cast<uint8_t>(std::min(255.0f, t * 290.0f));
            const uint8_t b = static_cast<uint8_t>(t * 190.0f);
            m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
        }
        break;
    }
    case 2: { // Sapphire Cosmic Ice
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            const uint8_t r = static_cast<uint8_t>(t * t * 180.0f);
            const uint8_t g = static_cast<uint8_t>(t * 220.0f);
            const uint8_t b = static_cast<uint8_t>(std::min(255.0f, 60.0f + t * 240.0f));
            m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
        }
        break;
    }
    case 3: { // Amethyst Violet
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            const uint8_t r = static_cast<uint8_t>(t * 255.0f);
            const uint8_t g = static_cast<uint8_t>(t * t * 110.0f);
            const uint8_t b = static_cast<uint8_t>(std::min(255.0f, t * 280.0f));
            m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
        }
        break;
    }
    }
}

void glenz_vector_effect::on_enter() {
    m_canvas.clear(0);
    init_palette();
    m_orientation = euler::quaternion<float>::identity();
    m_angular_velocity = {0.6f, 0.9f, 0.4f};
    m_zoom = 160.0f;
    m_time = 0.0f;
}

void glenz_vector_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    if (in.pressed(sdlpp::scancode::space)) {
        m_poly_type = static_cast<PolyType>((static_cast<int>(m_poly_type) + 1) % static_cast<int>(PolyType::Count));
    }

    if (in.pressed(sdlpp::scancode::c)) {
        m_theme = (m_theme + 1) % 4;
        init_palette();
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

void glenz_vector_effect::draw_glenz_polygon(const std::vector<std::pair<int, int>>& pts, uint8_t color, uint8_t* raw) {
    if (pts.size() < 3) return;

    int min_y = vga_canvas::height;
    int max_y = -1;
    for (const auto& [x, y] : pts) {
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
    }

    min_y = std::max(0, min_y);
    max_y = std::min(vga_canvas::height - 1, max_y);
    if (min_y > max_y) return;

    std::vector<int> x_intersects;
    x_intersects.reserve(8);

    const size_t n = pts.size();
    for (int y = min_y; y <= max_y; ++y) {
        x_intersects.clear();
        for (size_t i = 0; i < n; ++i) {
            const auto& p1 = pts[i];
            const auto& p2 = pts[(i + 1) % n];

            if ((p1.second <= y && p2.second > y) || (p2.second <= y && p1.second > y)) {
                const float dy = static_cast<float>(p2.second - p1.second);
                const float t = static_cast<float>(y - p1.second) / dy;
                const int intersect_x = static_cast<int>(static_cast<float>(p1.first) + t * static_cast<float>(p2.first - p1.first));
                x_intersects.push_back(intersect_x);
            }
        }

        std::sort(x_intersects.begin(), x_intersects.end());

        for (size_t i = 0; i + 1 < x_intersects.size(); i += 2) {
            int x1 = std::max(0, x_intersects[i]);
            int x2 = std::min(vga_canvas::width - 1, x_intersects[i + 1]);

            for (int x = x1; x <= x2; ++x) {
                const int idx = y * vga_canvas::width + x;
                const uint32_t val = static_cast<uint32_t>(raw[idx]) + color;
                raw[idx] = static_cast<uint8_t>(val > 255 ? 255 : val);
            }
        }
    }

    // Draw bright Glenz wireframe facet borders
    auto draw_line = [raw](int x0, int y0, int x1, int y1, uint8_t border_col) {
        int dx = std::abs(x1 - x0);
        int dy = -std::abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;

        while (true) {
            if (x0 >= 0 && x0 < vga_canvas::width && y0 >= 0 && y0 < vga_canvas::height) {
                const int idx = y0 * vga_canvas::width + x0;
                const uint32_t val = static_cast<uint32_t>(raw[idx]) + border_col;
                raw[idx] = static_cast<uint8_t>(val > 255 ? 255 : val);
            }
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    };

    for (size_t i = 0; i < n; ++i) {
        const auto& p1 = pts[i];
        const auto& p2 = pts[(i + 1) % n];
        draw_line(p1.first, p1.second, p2.first, p2.second, 70);
    }
}

void glenz_vector_effect::render(const neutrino::rect& viewport) {
    m_canvas.clear(0);
    uint8_t* raw = m_canvas.raw_pixels();

    const auto& model = m_models[static_cast<size_t>(m_poly_type)];
    constexpr float camera_z = 220.0f;

    const auto mat = m_orientation.to_matrix3();
    const float r00 = mat(0, 0), r01 = mat(0, 1), r02 = mat(0, 2);
    const float r10 = mat(1, 0), r11 = mat(1, 1), r12 = mat(1, 2);
    const float r20 = mat(2, 0), r21 = mat(2, 1), r22 = mat(2, 2);

    // Transform and project all vertices
    std::vector<std::pair<int, int>> screen_pts(model.vertices.size());
    std::vector<float> vertex_z(model.vertices.size());

    for (size_t i = 0; i < model.vertices.size(); ++i) {
        const auto& pt = model.vertices[i];
        const float x = pt.x();
        const float y = pt.y();
        const float z = pt.z();

        const float pz = r20 * x + r21 * y + r22 * z + camera_z;
        vertex_z[i] = pz;

        const float inv_z = (pz > 10.0f) ? (m_zoom / pz) : 1.0f;
        screen_pts[i].first = 160 + static_cast<int>((r00 * x + r01 * y + r02 * z) * inv_z);
        screen_pts[i].second = 100 + static_cast<int>((r10 * x + r11 * y + r12 * z) * inv_z);
    }

    // Sort all faces by average Z (Painter's algorithm back-to-front so back-faces blend through)
    struct FaceZ {
        size_t face_index;
        float avg_z;
    };
    std::vector<FaceZ> sorted_faces(model.faces.size());

    for (size_t fi = 0; fi < model.faces.size(); ++fi) {
        const auto& face = model.faces[fi];
        float sum_z = 0.0f;
        for (int vi : face.vertex_indices) {
            sum_z += vertex_z[static_cast<size_t>(vi)];
        }
        sorted_faces[fi] = {fi, sum_z / static_cast<float>(face.vertex_indices.size())};
    }

    std::sort(sorted_faces.begin(), sorted_faces.end(), [](const FaceZ& a, const FaceZ& b) {
        return a.avg_z > b.avg_z; // Back faces first
    });

    // Render each translucent face with additive blending!
    std::vector<std::pair<int, int>> poly_pts;
    for (const auto& fz : sorted_faces) {
        const auto& face = model.faces[fz.face_index];
        poly_pts.clear();
        for (int vi : face.vertex_indices) {
            poly_pts.push_back(screen_pts[static_cast<size_t>(vi)]);
        }
        draw_glenz_polygon(poly_pts, face.base_color, raw);
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
