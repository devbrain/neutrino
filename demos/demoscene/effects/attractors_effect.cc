#include "attractors_effect.hh"

#include <algorithm>
#include <cmath>
#if defined(__x86_64__) || defined(_M_X64) || defined(__SSE2__)
#include <emmintrin.h>
#endif

namespace demoscene {

attractors_effect::attractors_effect()
    : m_points(max_points, euler::vec3<float>{0.0f, 0.0f, 0.0f})
{
    on_enter();
}

std::string_view attractors_effect::math_formula() const noexcept {
    switch (m_attractor_type) {
    case 0: return "dx/dt = σ(y-x), dy/dt = x(ρ-z)-y, dz/dt = xy-βz (Lorenz 1963)";
    case 1: return "x' = sin(ay) - z·cos(bx), y' = z·sin(cx) - cos(dy) (Pickover 3D)";
    case 2: return "dx/dt = -y-z, dy/dt = x+ay, dz/dt = b+z(x-c) (Rössler 1976)";
    case 3: return "dx/dt = (z-b)x - dy, dy/dt = dx + (z-b)y, dz/dt = c+az-z³/3... (Aizawa)";
    default: return "Chaotic Differential Dynamical System";
    }
}

std::string_view attractors_effect::description() const noexcept {
    switch (m_attractor_type) {
    case 0: return "Lorenz Strange Attractor: The classic butterfly effect discovered in atmospheric convection.";
    case 1: return "Pickover 3D Attractor: Discrete non-linear trigonometric iterative map with folded fractal geometry.";
    case 2: return "Rössler Attractor: Continuous dynamical spiral band with a chaotic topological fold.";
    case 3: return "Aizawa Attractor: Toroidal spiral attractor modeling non-linear vortex sphere turbulence.";
    default: return "3D Strange Attractor";
    }
}

void attractors_effect::on_enter() {
    m_canvas.clear(0);
    init_palette();
    reset_simulation();
}

void attractors_effect::init_palette() {
    m_canvas.set_rgb(0, 0, 0, 0); // Background black

    switch (m_palette_theme) {
    case 0: { // Electric Neon Blue / Cyan CRT
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.6f) {
                const float u = t / 0.6f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(u * 30.0f),
                    static_cast<uint8_t>(40.0f + u * 180.0f),
                    static_cast<uint8_t>(80.0f + u * 175.0f));
            } else {
                const float u = (t - 0.6f) / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(30.0f + u * 225.0f),
                    static_cast<uint8_t>(220.0f + u * 35.0f),
                    255);
            }
        }
        break;
    }
    case 1: { // Phosphor Emerald Oscilloscope
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.7f) {
                const float u = t / 0.7f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(u * 20.0f),
                    static_cast<uint8_t>(30.0f + u * 225.0f),
                    static_cast<uint8_t>(u * 40.0f));
            } else {
                const float u = (t - 0.7f) / 0.3f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(20.0f + u * 235.0f),
                    255,
                    static_cast<uint8_t>(40.0f + u * 215.0f));
            }
        }
        break;
    }
    case 2: { // Solar Amber / Magma Flame
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.4f) {
                const float u = t / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(u * 180.0f),
                    0,
                    0);
            } else if (t < 0.8f) {
                const float u = (t - 0.4f) / 0.4f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(180.0f + u * 75.0f),
                    static_cast<uint8_t>(u * 190.0f),
                    0);
            } else {
                const float u = (t - 0.8f) / 0.2f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    255,
                    static_cast<uint8_t>(190.0f + u * 65.0f),
                    static_cast<uint8_t>(u * 255.0f));
            }
        }
        break;
    }
    case 3: { // Cyberpunk Twilight (Deep Violet -> Magenta -> Cyan)
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            if (t < 0.5f) {
                const float u = t / 0.5f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(u * 230.0f),
                    static_cast<uint8_t>(u * 30.0f),
                    static_cast<uint8_t>(60.0f + u * 180.0f));
            } else {
                const float u = (t - 0.5f) / 0.5f;
                m_canvas.set_rgb(static_cast<uint8_t>(i),
                    static_cast<uint8_t>(230.0f + u * 25.0f),
                    static_cast<uint8_t>(30.0f + u * 225.0f),
                    255);
            }
        }
        break;
    }
    }
}

void attractors_effect::reset_simulation() {
    switch (m_attractor_type) {
    case 0: // Lorenz
        m_current_pos = {0.1f, 0.0f, 0.0f};
        m_zoom = 160.0f;
        break;
    case 1: // Pickover
        m_current_pos = {0.5f, 0.5f, 0.5f};
        m_zoom = 180.0f;
        break;
    case 2: // Rossler
        m_current_pos = {0.1f, 0.0f, 0.0f};
        m_zoom = 140.0f;
        break;
    case 3: // Aizawa
        m_current_pos = {0.1f, 0.0f, 0.0f};
        m_zoom = 175.0f;
        break;
    }

    m_write_head = 0;
    m_valid_points = 0;
    step_simulation(static_cast<int>(max_points));
}

void attractors_effect::step_simulation(int steps) {
    for (int s = 0; s < steps; ++s) {
        euler::vec3<float> p = m_current_pos;
        euler::vec3<float> next_p = p;

        switch (m_attractor_type) {
        case 0: { // Lorenz
            constexpr float sigma = 10.0f;
            constexpr float rho = 28.0f;
            constexpr float beta = 8.0f / 3.0f;
            constexpr float dt = 0.008f;

            const float dx = sigma * (p.y() - p.x());
            const float dy = p.x() * (rho - p.z()) - p.y();
            const float dz = p.x() * p.y() - beta * p.z();

            next_p = {p.x() + dx * dt, p.y() + dy * dt, p.z() + dz * dt};
            break;
        }
        case 1: { // Pickover 3D
            constexpr float a = 2.24f;
            constexpr float b = 0.43f;
            constexpr float c = -0.65f;
            constexpr float d = -2.1f;

            const float nx = std::sin(a * p.y()) - p.z() * std::cos(b * p.x());
            const float ny = p.z() * std::sin(c * p.x()) - std::cos(d * p.y());
            const float nz = std::sin(p.x());

            next_p = {nx, ny, nz};
            break;
        }
        case 2: { // Rossler
            constexpr float a = 0.2f;
            constexpr float b = 0.2f;
            constexpr float c = 5.7f;
            constexpr float dt = 0.02f;

            const float dx = -p.y() - p.z();
            const float dy = p.x() + a * p.y();
            const float dz = b + p.z() * (p.x() - c);

            next_p = {p.x() + dx * dt, p.y() + dy * dt, p.z() + dz * dt};
            break;
        }
        case 3: { // Aizawa
            constexpr float a = 0.95f;
            constexpr float b = 0.7f;
            constexpr float c = 0.6f;
            constexpr float d = 3.5f;
            constexpr float e = 0.25f;
            constexpr float f = 0.1f;
            constexpr float dt = 0.01f;

            const float x = p.x(), y = p.y(), z = p.z();
            const float dx = (z - b) * x - d * y;
            const float dy = d * x + (z - b) * y;
            const float dz = c + a * z - (z * z * z) / 3.0f - (x * x + y * y) * (1.0f + e * z) + f * z * (x * x * x);

            next_p = {x + dx * dt, y + dy * dt, z + dz * dt};
            break;
        }
        }

        m_current_pos = next_p;
        m_points[m_write_head] = next_p;
        m_write_head = (m_write_head + 1) % max_points;
        if (m_valid_points < max_points) {
            m_valid_points++;
        }
    }
}

void attractors_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_sim_time += dt_sec;

    if (in.pressed(sdlpp::scancode::space)) {
        m_attractor_type = (m_attractor_type + 1) % 4;
        reset_simulation();
        m_canvas.clear(0);
    }

    if (in.pressed(sdlpp::scancode::a)) {
        m_attenuate = !m_attenuate;
    }

    if (in.pressed(sdlpp::scancode::c)) {
        m_palette_theme = (m_palette_theme + 1) % 4;
        init_palette();
    }

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }

    if (in.pressed(sdlpp::scancode::equals) || in.pressed(sdlpp::scancode::kp_plus)) {
        m_active_points = std::min(max_points, m_active_points + 1024);
    }
    if (in.pressed(sdlpp::scancode::minus) || in.pressed(sdlpp::scancode::kp_minus)) {
        m_active_points = std::max(size_t{1024}, m_active_points - 1024);
    }

    if (in.held(sdlpp::scancode::w)) m_zoom = std::min(400.0f, m_zoom + 120.0f * dt_sec);
    if (in.held(sdlpp::scancode::s)) m_zoom = std::max(60.0f, m_zoom - 120.0f * dt_sec);

    if (in.held(sdlpp::scancode::left))  m_angular_velocity[1] -= 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::right)) m_angular_velocity[1] += 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::up))    m_angular_velocity[0] -= 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::down))  m_angular_velocity[0] += 2.0f * dt_sec;

    // Quaternion auto-tumble and interactive orientation
    const float speed = m_angular_velocity.length();
    if (speed > 1e-4f) {
        const euler::vec3<float> axis = m_angular_velocity.normalized();
        const euler::radian<float> angle_delta = euler::radian<float>(speed * dt_sec);
        const euler::quaternion<float> delta_rot = euler::quaternion<float>::from_axis_angle(axis, angle_delta);
        m_orientation = (delta_rot * m_orientation).normalized();
    }

    // Step simulation in real-time
    step_simulation(80);
}

void attractors_effect::render(const neutrino::rect& viewport) {
    uint8_t* raw = m_canvas.raw_pixels();

    // 1. CRT Phosphor Decay Attenuation or Clear Screen
    if (m_attenuate) {
#if defined(__x86_64__) || defined(_M_X64) || defined(__SSE2__)
        // Vectorized SIMD decay (~82% per frame) for all 64,000 pixels in 4,000 cycles
        const __m128i factor = _mm_set1_epi16(210);
        const __m128i zero = _mm_setzero_si128();
        __m128i* ptr = reinterpret_cast<__m128i*>(raw);
        constexpr size_t count16 = vga_canvas::pixel_count / 16;
        for (size_t i = 0; i < count16; ++i) {
            const __m128i chunk = _mm_loadu_si128(ptr + i);
            __m128i lo = _mm_unpacklo_epi8(chunk, zero);
            __m128i hi = _mm_unpackhi_epi8(chunk, zero);
            lo = _mm_srli_epi16(_mm_mullo_epi16(lo, factor), 8);
            hi = _mm_srli_epi16(_mm_mullo_epi16(hi, factor), 8);
            _mm_storeu_si128(ptr + i, _mm_packus_epi16(lo, hi));
        }
#else
        static const auto decay_lut = [] {
            std::array<uint8_t, 256> lut{};
            for (int i = 0; i < 256; ++i) {
                lut[i] = static_cast<uint8_t>((i * 210) >> 8);
            }
            return lut;
        }();
        for (int i = 0; i < vga_canvas::pixel_count; ++i) {
            raw[i] = decay_lut[raw[i]];
        }
#endif
    } else {
        m_canvas.clear(0);
    }

    // 2. Attractor centering and scale normalization
    euler::vec3<float> center_offset{0.0f, 0.0f, 0.0f};
    float scale = 1.0f;
    constexpr float camera_z = 240.0f;

    switch (m_attractor_type) {
    case 0: // Lorenz
        center_offset = {0.0f, 0.0f, -25.0f};
        scale = 4.6f;
        break;
    case 1: // Pickover
        center_offset = {0.0f, 0.0f, 0.0f};
        scale = 55.0f;
        break;
    case 2: // Rossler
        center_offset = {0.0f, 0.0f, -12.0f};
        scale = 7.0f;
        break;
    case 3: // Aizawa
        center_offset = {0.0f, 0.0f, 0.0f};
        scale = 52.0f;
        break;
    }

    // 3. Hoist 3D Rotation Matrix & Pre-fold scaling & translation
    const auto mat = m_orientation.to_matrix3();
    const float r00 = mat(0, 0) * scale, r01 = mat(0, 1) * scale, r02 = mat(0, 2) * scale;
    const float r10 = mat(1, 0) * scale, r11 = mat(1, 1) * scale, r12 = mat(1, 2) * scale;
    const float r20 = mat(2, 0) * scale, r21 = mat(2, 1) * scale, r22 = mat(2, 2) * scale;

    const float cx = center_offset.x();
    const float cy = center_offset.y();
    const float cz = center_offset.z();

    const float tx = r00 * cx + r01 * cy + r02 * cz;
    const float ty = r10 * cx + r11 * cy + r12 * cz;
    const float tz = r20 * cx + r21 * cy + r22 * cz + camera_z;

    auto add_sat = [](uint8_t* p, uint8_t delta) {
        const uint32_t val = static_cast<uint32_t>(*p) + delta;
        *p = static_cast<uint8_t>(val > 255 ? 255 : val);
    };

    // 4. Render particle trajectory with fast direct pointer splatting
    const size_t total = std::min(m_valid_points, m_active_points);
    for (size_t k = 0; k < total; ++k) {
        const auto& pt = m_points[k];
        const float px = pt.x();
        const float py = pt.y();
        const float pz_raw = pt.z();

        const float pz = r20 * px + r21 * py + r22 * pz_raw + tz;
        if (pz < 10.0f) continue;

        const float inv_z = m_zoom / pz;
        const int sx = 160 + static_cast<int>((r00 * px + r01 * py + r02 * pz_raw + tx) * inv_z);
        const int sy = 100 + static_cast<int>((r10 * px + r11 * py + r12 * pz_raw + ty) * inv_z);

        if (sx >= 1 && sx < vga_canvas::width - 1 && sy >= 1 && sy < vga_canvas::height - 1) {
            uint8_t* const p_mid = raw + sy * vga_canvas::width + sx;
            uint8_t* const p_top = p_mid - vga_canvas::width;
            uint8_t* const p_bot = p_mid + vga_canvas::width;

            add_sat(p_top - 1, 8);
            add_sat(p_top,     16);
            add_sat(p_top + 1, 8);

            add_sat(p_mid - 1, 16);
            add_sat(p_mid,     32);
            add_sat(p_mid + 1, 16);

            add_sat(p_bot - 1, 8);
            add_sat(p_bot,     16);
            add_sat(p_bot + 1, 8);
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
