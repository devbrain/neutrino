#include "dotmorph_effect.hh"

#include <algorithm>
#include <cmath>
#include <numbers>

#if defined(__x86_64__) || defined(_M_X64) || defined(__SSE2__)
#include <emmintrin.h>
#endif

namespace demoscene {

dotmorph_effect::dotmorph_effect()
    : m_shapes(SHAPE_COUNT, std::vector<point3f>(max_particles))
{
    generate_shapes();
    on_enter();
}

void dotmorph_effect::generate_shapes() {
    constexpr float pi = static_cast<float>(std::numbers::pi);
    constexpr float pi2 = pi * 2.0f;
    constexpr float golden_angle = pi * (3.0f - std::sqrt(5.0f));

    // 1. Sphere (Fibonacci Spiral)
    constexpr float sphere_radius = 65.0f;
    for (size_t i = 0; i < max_particles; ++i) {
        const float y = 1.0f - (static_cast<float>(i) / static_cast<float>(max_particles - 1)) * 2.0f;
        const float radius_at_y = std::sqrt(std::max(0.0f, 1.0f - y * y));
        const float theta = golden_angle * static_cast<float>(i);

        const float x = std::cos(theta) * radius_at_y * sphere_radius;
        const float z = std::sin(theta) * radius_at_y * sphere_radius;
        m_shapes[SHAPE_SPHERE][i] = point3f{x, y * sphere_radius, z};
    }

    // 2. Torus (Donut)
    constexpr float major_r = 52.0f;
    constexpr float minor_r = 22.0f;
    for (size_t i = 0; i < max_particles; ++i) {
        const float u = static_cast<float>(i % 64) / 64.0f * pi2;
        const float v = static_cast<float>(i / 64) / static_cast<float>(max_particles / 64) * pi2;

        const float x = (major_r + minor_r * std::cos(u)) * std::cos(v);
        const float y = (major_r + minor_r * std::cos(u)) * std::sin(v);
        const float z = minor_r * std::sin(u);
        m_shapes[SHAPE_TORUS][i] = point3f{x, y, z};
    }

    // 3. Cube
    constexpr float s = 48.0f;
    for (size_t i = 0; i < max_particles; ++i) {
        const int face = static_cast<int>(i % 6);
        const float u = ((static_cast<float>((i * 7) % 64) / 63.0f) * 2.0f - 1.0f) * s;
        const float v = ((static_cast<float>((i * 13) % 64) / 63.0f) * 2.0f - 1.0f) * s;

        switch (face) {
        case 0: m_shapes[SHAPE_CUBE][i] = point3f{ s,  u,  v}; break;
        case 1: m_shapes[SHAPE_CUBE][i] = point3f{-s,  u,  v}; break;
        case 2: m_shapes[SHAPE_CUBE][i] = point3f{ u,  s,  v}; break;
        case 3: m_shapes[SHAPE_CUBE][i] = point3f{ u, -s,  v}; break;
        case 4: m_shapes[SHAPE_CUBE][i] = point3f{ u,  v,  s}; break;
        default:m_shapes[SHAPE_CUBE][i] = point3f{ u,  v, -s}; break;
        }
    }

    // 4. Double Helix (DNA Strand)
    for (size_t i = 0; i < max_particles; ++i) {
        const float t = (static_cast<float>(i) / static_cast<float>(max_particles)) * pi * 8.0f - (pi * 4.0f);
        const float strand = (i % 2 == 0) ? 0.0f : pi;
        constexpr float helix_r = 38.0f;

        const float x = std::cos(t + strand) * helix_r;
        const float y = (t / (pi * 4.0f)) * 75.0f;
        const float z = std::sin(t + strand) * helix_r;
        m_shapes[SHAPE_HELIX][i] = point3f{x, y, z};
    }

    // 5. Trefoil Knot
    for (size_t i = 0; i < max_particles; ++i) {
        const float t = (static_cast<float>(i) / static_cast<float>(max_particles)) * pi2;
        const float x = (std::sin(t) + 2.0f * std::sin(2.0f * t)) * 24.0f;
        const float y = (std::cos(t) - 2.0f * std::cos(2.0f * t)) * 24.0f;
        const float z = (-std::sin(3.0f * t)) * 32.0f;
        m_shapes[SHAPE_TREFOIL][i] = point3f{x, y, z};
    }
}

void dotmorph_effect::on_enter() {
    m_canvas.clear(0);
    init_palette();
    m_source_shape = 0;
    m_target_shape = 1;
    m_morph_progress = 0.0f;
    m_auto_morph = true;
    m_orientation = euler::quaternion<float>::identity();
    m_angular_velocity = {0.5f, 1.1f, 0.35f};
    m_zoom = 150.0f;
}

void dotmorph_effect::init_palette() {
    m_canvas.set_rgb_6bit(0, 0, 0, 0); // Cosmic void

    switch (m_theme) {
    case 0: { // Liquid Chrome Cyan
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(u * 40.0f),
                    static_cast<uint8_t>(60.0f + u * 160.0f),
                    static_cast<uint8_t>(100.0f + u * 155.0f));
            } else {
                const float u = (t - 0.6f) / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(40.0f + u * 215.0f),
                    static_cast<uint8_t>(220.0f + u * 35.0f),
                    255);
            }
        }
        break;
    }
    case 1: { // Solar Gold Flame
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(std::min(255.0f, t * 300.0f)),
                static_cast<uint8_t>(t * t * 220.0f),
                static_cast<uint8_t>(t * t * t * 100.0f));
        }
        break;
    }
    case 2: { // Cyberpunk Magenta Neon
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(t * 255.0f),
                static_cast<uint8_t>(t * t * 80.0f),
                static_cast<uint8_t>(80.0f + t * 175.0f));
        }
        break;
    }
    case 3: { // Matrix Phosphor Emerald
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(t * t * 120.0f),
                static_cast<uint8_t>(40.0f + t * 215.0f),
                static_cast<uint8_t>(t * t * 60.0f));
        }
        break;
    }
    }
}

void dotmorph_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    if (in.pressed(sdlpp::scancode::space)) {
        m_source_shape = m_target_shape;
        m_target_shape = (m_target_shape + 1) % SHAPE_COUNT;
        m_morph_progress = 0.0f;
    }

    if (in.pressed(sdlpp::scancode::equals) || in.pressed(sdlpp::scancode::kp_plus)) {
        m_active_particles = std::min(max_particles, m_active_particles + 1024);
    }
    if (in.pressed(sdlpp::scancode::minus) || in.pressed(sdlpp::scancode::kp_minus)) {
        m_active_particles = std::max(size_t{1024}, m_active_particles - 1024);
    }

    if (in.pressed(sdlpp::scancode::rightbracket)) {
        m_morph_speed = std::min(3.0f, m_morph_speed + 0.3f);
    }
    if (in.pressed(sdlpp::scancode::leftbracket)) {
        m_morph_speed = std::max(0.4f, m_morph_speed - 0.3f);
    }

    if (in.pressed(sdlpp::scancode::a)) {
        m_phosphor_trails = !m_phosphor_trails;
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

    // Automatic morph cycle: morph (0..1) -> hold shape (1..2) -> next
    if (m_auto_morph) {
        m_morph_progress += dt_sec * m_morph_speed;
        if (m_morph_progress >= 2.0f) {
            m_source_shape = m_target_shape;
            m_target_shape = (m_target_shape + 1) % SHAPE_COUNT;
            m_morph_progress = 0.0f;
        }
    }

    // Quaternion auto-tumble
    const float speed = m_angular_velocity.length();
    if (speed > 1e-4f) {
        const euler::vec3<float> axis = m_angular_velocity.normalized();
        const euler::radian<float> angle_delta = euler::radian<float>(speed * dt_sec);
        const euler::quaternion<float> delta_rot = euler::quaternion<float>::from_axis_angle(axis, angle_delta);
        m_orientation = (delta_rot * m_orientation).normalized();
    }
}

void dotmorph_effect::render(const neutrino::rect& viewport) {
    uint8_t* raw = m_canvas.raw_pixels();

    if (m_phosphor_trails) {
#if defined(__x86_64__) || defined(_M_X64) || defined(__SSE2__)
        const __m128i factor = _mm_set1_epi16(210);
        const __m128i zero = _mm_setzero_si128();
        __m128i* ptr = reinterpret_cast<__m128i*>(raw);
        constexpr size_t count16 = vga_canvas::pixel_count / 16;
        for (size_t i = 0; i < count16; ++i) {
            __m128i px = _mm_loadu_si128(ptr + i);
            __m128i lo = _mm_unpacklo_epi8(px, zero);
            __m128i hi = _mm_unpackhi_epi8(px, zero);
            lo = _mm_srli_epi16(_mm_mullo_epi16(lo, factor), 8);
            hi = _mm_srli_epi16(_mm_mullo_epi16(hi, factor), 8);
            _mm_storeu_si128(ptr + i, _mm_packus_epi16(lo, hi));
        }
#else
        for (int i = 0; i < vga_canvas::pixel_count; ++i) {
            raw[i] = static_cast<uint8_t>((static_cast<uint32_t>(raw[i]) * 210) >> 8);
        }
#endif
    } else {
        m_canvas.clear(0);
    }

    constexpr float camera_z = 240.0f;

    // Hoist rotation matrix
    const auto mat = m_orientation.to_matrix3();
    const float r00 = mat(0, 0), r01 = mat(0, 1), r02 = mat(0, 2);
    const float r10 = mat(1, 0), r11 = mat(1, 1), r12 = mat(1, 2);
    const float r20 = mat(2, 0), r21 = mat(2, 1), r22 = mat(2, 2);

    const bool is_morphing = (m_morph_progress < 1.0f);
    const float alpha_clamped = std::clamp(m_morph_progress, 0.0f, 1.0f);
    const float smooth_alpha = 0.5f * (1.0f - std::cos(alpha_clamped * static_cast<float>(std::numbers::pi)));

    const auto* src = m_shapes[m_source_shape].data();
    const auto* dst = m_shapes[m_target_shape].data();
    const size_t total_pts = m_active_particles;

    for (size_t i = 0; i < total_pts; ++i) {
        float x = src[i].x;
        float y = src[i].y;
        float z = src[i].z;

        if (is_morphing) {
            x += (dst[i].x - x) * smooth_alpha;
            y += (dst[i].y - y) * smooth_alpha;
            z += (dst[i].z - z) * smooth_alpha;
        }

        const float pz = r20 * x + r21 * y + r22 * z + camera_z;
        if (pz < 10.0f) continue;

        const float inv_z = m_zoom / pz;
        const int sx = 160 + static_cast<int>((r00 * x + r01 * y + r02 * z) * inv_z);
        const int sy = 100 + static_cast<int>((r10 * x + r11 * y + r12 * z) * inv_z);

        if (sx >= 0 && sx < vga_canvas::width && sy >= 0 && sy < vga_canvas::height) {
            const int depth_col = static_cast<int>((280.0f - pz) * 1.6f);
            const uint8_t col = static_cast<uint8_t>(std::clamp(depth_col, 20, 255));

            raw[sy * vga_canvas::width + sx] = col;

            // Highlight specular foreground point
            if (col > 220 && sx + 1 < vga_canvas::width && sy + 1 < vga_canvas::height) {
                raw[sy * vga_canvas::width + sx + 1] = static_cast<uint8_t>(col / 2);
                raw[(sy + 1) * vga_canvas::width + sx] = static_cast<uint8_t>(col / 2);
            }
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
