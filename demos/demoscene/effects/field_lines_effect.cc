#include "field_lines_effect.hh"

#include <algorithm>
#include <cmath>
#include <numbers>

#if defined(__SSE2__)
#include <immintrin.h>
#endif

namespace demoscene {

field_lines_effect::field_lines_effect() {
    // Precompute 1024-entry sine LUT for fast potential contour calculation
    constexpr float two_pi = 2.0f * std::numbers::pi_v<float>;
    for (size_t i = 0; i < m_sin_lut.size(); ++i) {
        const float angle = static_cast<float>(i) * (two_pi / static_cast<float>(m_sin_lut.size()));
        m_sin_lut[i] = std::sin(angle);
    }

    on_enter();
}

void field_lines_effect::init_palette() {
    m_canvas.set_rgb(0, 8, 10, 18); // Dark laboratory background

    switch (m_color_theme) {
    case 0: {
        // Electrostatic Cyan & Amber:
        // 1..100: Negative potential (Blue / Cyan)
        for (int i = 1; i <= 100; ++i) {
            const float t = static_cast<float>(i) / 100.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(t * 40),
                static_cast<uint8_t>(40 + t * 140),
                static_cast<uint8_t>(80 + t * 175));
        }
        // 101..200: Positive potential (Red / Amber Gold)
        for (int i = 101; i <= 200; ++i) {
            const float t = static_cast<float>(i - 101) / 99.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(100 + t * 155),
                static_cast<uint8_t>(30 + t * 140),
                static_cast<uint8_t>(t * 30));
        }
        // 201..240: Electric Field Lines (Blinding Neon Green/White)
        for (int i = 201; i <= 240; ++i) {
            const float t = static_cast<float>(i - 201) / 39.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(80 + t * 175),
                255,
                static_cast<uint8_t>(120 + t * 135));
        }
        // 241..255: Charge markers (+ / - cores)
        for (int i = 241; i <= 255; ++i) {
            m_canvas.set_rgb(static_cast<uint8_t>(i), 255, 255, 255);
        }
        break;
    }
    case 1: {
        // Deep Space Violet & Neon Magenta:
        for (int i = 1; i <= 100; ++i) {
            const float t = static_cast<float>(i) / 100.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(60 + t * 120),
                static_cast<uint8_t>(t * 30),
                static_cast<uint8_t>(100 + t * 155));
        }
        for (int i = 101; i <= 200; ++i) {
            const float t = static_cast<float>(i - 101) / 99.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(180 + t * 75),
                static_cast<uint8_t>(20 + t * 60),
                static_cast<uint8_t>(120 + t * 100));
        }
        for (int i = 201; i <= 240; ++i) {
            m_canvas.set_rgb(static_cast<uint8_t>(i), 255, 240, 100); // Gold lines
        }
        for (int i = 241; i <= 255; ++i) {
            m_canvas.set_rgb(static_cast<uint8_t>(i), 255, 255, 255);
        }
        break;
    }
    default:
        break;
    }
}

void field_lines_effect::reset_charges() {
    m_charges.clear();
    // 4 dynamic point charges: 2 positive, 2 negative
    m_charges.push_back({euler::vec2<float>{100.0f,  70.0f}, +1.0f, 6.0f});
    m_charges.push_back({euler::vec2<float>{220.0f, 130.0f}, -1.0f, 6.0f});
    m_charges.push_back({euler::vec2<float>{220.0f,  70.0f}, +1.0f, 6.0f});
    m_charges.push_back({euler::vec2<float>{100.0f, 130.0f}, -1.0f, 6.0f});
}

void field_lines_effect::on_enter() {
    init_palette();
    reset_charges();
    m_time = 0.0f;
    m_display_mode = 0;
    m_color_theme = 0;
}

void field_lines_effect::compute_field_fast(float px, float py, float& ex, float& ey) const noexcept {
    ex = 0.0f;
    ey = 0.0f;

    for (const auto& c : m_charges) {
        const float dx = px - c.pos.x();
        const float dy = py - c.pos.y();
        const float r2 = dx * dx + dy * dy + 4.0f; // Softening epsilon
        const float inv_r = 1.0f / std::sqrt(r2);
        const float scale = c.q * 1000.0f * (inv_r / r2);

        ex += dx * scale;
        ey += dy * scale;
    }
}

void field_lines_effect::trace_line(float start_x, float start_y, float sign) {
    constexpr int max_steps = 140;
    constexpr float step_size = 1.5f;

    float cur_x = start_x;
    float cur_y = start_y;

    // Cache sink charges (negative charges) for rapid termination check
    const auto& c1 = m_charges[1];
    const auto& c3 = m_charges[3];
    const float sink1_x = c1.pos.x(), sink1_y = c1.pos.y(), sink1_r2 = c1.radius * c1.radius;
    const float sink3_x = c3.pos.x(), sink3_y = c3.pos.y(), sink3_r2 = c3.radius * c3.radius;

    uint8_t* raw = m_canvas.raw_pixels();

    for (int step = 0; step < max_steps; ++step) {
        const int ix = static_cast<int>(cur_x);
        const int iy = static_cast<int>(cur_y);

        if (static_cast<unsigned>(ix) >= vga_canvas::width ||
            static_cast<unsigned>(iy) >= vga_canvas::height) break;

        // Check if field line terminated inside negative sink charges
        const float d1x = cur_x - sink1_x;
        const float d1y = cur_y - sink1_y;
        if (d1x * d1x + d1y * d1y < sink1_r2) break;

        const float d3x = cur_x - sink3_x;
        const float d3y = cur_y - sink3_y;
        if (d3x * d3x + d3y * d3y < sink3_r2) break;

        const uint8_t line_color = static_cast<uint8_t>(201 + (step * 39 / max_steps));
        raw[iy * vga_canvas::width + ix] = line_color;

        float ex = 0.0f, ey = 0.0f;
        compute_field_fast(cur_x, cur_y, ex, ey);

        const float len2 = ex * ex + ey * ey;
        if (len2 < 1e-6f) break;

        const float inv_len = (1.0f / std::sqrt(len2)) * (sign * step_size);
        cur_x += ex * inv_len;
        cur_y += ey * inv_len;
    }
}

void field_lines_effect::render_heatmap() {
    uint8_t* raw = m_canvas.raw_pixels();
    constexpr float rad_to_lut = 1024.0f / (2.0f * std::numbers::pi_v<float>);

    const float cx0 = m_charges[0].pos.x(), cy0 = m_charges[0].pos.y(), q0_scaled = m_charges[0].q * 120.0f;
    const float cx1 = m_charges[1].pos.x(), cy1 = m_charges[1].pos.y(), q1_scaled = m_charges[1].q * 120.0f;
    const float cx2 = m_charges[2].pos.x(), cy2 = m_charges[2].pos.y(), q2_scaled = m_charges[2].q * 120.0f;
    const float cx3 = m_charges[3].pos.x(), cy3 = m_charges[3].pos.y(), q3_scaled = m_charges[3].q * 120.0f;

#if defined(__SSE2__)
    const __m128 v_cx0 = _mm_set1_ps(cx0), v_cy0 = _mm_set1_ps(cy0), v_q0 = _mm_set1_ps(q0_scaled);
    const __m128 v_cx1 = _mm_set1_ps(cx1), v_cy1 = _mm_set1_ps(cy1), v_q1 = _mm_set1_ps(q1_scaled);
    const __m128 v_cx2 = _mm_set1_ps(cx2), v_cy2 = _mm_set1_ps(cy2), v_q2 = _mm_set1_ps(q2_scaled);
    const __m128 v_cx3 = _mm_set1_ps(cx3), v_cy3 = _mm_set1_ps(cy3), v_q3 = _mm_set1_ps(q3_scaled);

    const __m128 x_offsets = _mm_set_ps(6.0f, 4.0f, 2.0f, 0.0f);
    const __m128 eps = _mm_set1_ps(4.0f);

    for (int y = 0; y < vga_canvas::height; y += 2) {
        const __m128 fy = _mm_set1_ps(static_cast<float>(y));
        const __m128 dy0 = _mm_sub_ps(fy, v_cy0);
        const __m128 dy1 = _mm_sub_ps(fy, v_cy1);
        const __m128 dy2 = _mm_sub_ps(fy, v_cy2);
        const __m128 dy3 = _mm_sub_ps(fy, v_cy3);

        const __m128 dy0_sq = _mm_add_ps(_mm_mul_ps(dy0, dy0), eps);
        const __m128 dy1_sq = _mm_add_ps(_mm_mul_ps(dy1, dy1), eps);
        const __m128 dy2_sq = _mm_add_ps(_mm_mul_ps(dy2, dy2), eps);
        const __m128 dy3_sq = _mm_add_ps(_mm_mul_ps(dy3, dy3), eps);

        uint8_t* row0 = raw + y * vga_canvas::width;
        uint8_t* row1 = row0 + vga_canvas::width;

        for (int x = 0; x < vga_canvas::width; x += 8) {
            const __m128 fx = _mm_add_ps(_mm_set1_ps(static_cast<float>(x)), x_offsets);

            const __m128 dx0 = _mm_sub_ps(fx, v_cx0);
            const __m128 dx1 = _mm_sub_ps(fx, v_cx1);
            const __m128 dx2 = _mm_sub_ps(fx, v_cx2);
            const __m128 dx3 = _mm_sub_ps(fx, v_cx3);

            const __m128 r0_sq = _mm_add_ps(_mm_mul_ps(dx0, dx0), dy0_sq);
            const __m128 r1_sq = _mm_add_ps(_mm_mul_ps(dx1, dx1), dy1_sq);
            const __m128 r2_sq = _mm_add_ps(_mm_mul_ps(dx2, dx2), dy2_sq);
            const __m128 r3_sq = _mm_add_ps(_mm_mul_ps(dx3, dx3), dy3_sq);

            // Single-cycle hardware reciprocal square root
            const __m128 inv_r0 = _mm_rsqrt_ps(r0_sq);
            const __m128 inv_r1 = _mm_rsqrt_ps(r1_sq);
            const __m128 inv_r2 = _mm_rsqrt_ps(r2_sq);
            const __m128 inv_r3 = _mm_rsqrt_ps(r3_sq);

            const __m128 pot = _mm_add_ps(
                _mm_add_ps(_mm_mul_ps(v_q0, inv_r0), _mm_mul_ps(v_q1, inv_r1)),
                _mm_add_ps(_mm_mul_ps(v_q2, inv_r2), _mm_mul_ps(v_q3, inv_r3))
            );

            alignas(16) float p[4];
            _mm_store_ps(p, pot);

            for (int k = 0; k < 4; ++k) {
                const float potential = p[k];
                const int lut_idx = static_cast<int>(potential * (1.5f * rad_to_lut)) & 1023;
                const float band = m_sin_lut[lut_idx];

                uint8_t col = 0;
                if (potential > 0.0f) {
                    col = static_cast<uint8_t>(101 + std::clamp(static_cast<int>((potential * 3.0f) + band * 15.0f), 0, 98));
                } else {
                    col = static_cast<uint8_t>(1 + std::clamp(static_cast<int>((-potential * 3.0f) + band * 15.0f), 0, 98));
                }

                const uint16_t double_pixel = static_cast<uint16_t>(col | (col << 8));
                const int px = x + k * 2;
                *reinterpret_cast<uint16_t*>(row0 + px) = double_pixel;
                *reinterpret_cast<uint16_t*>(row1 + px) = double_pixel;
            }
        }
    }
#else
    // Portable hoisted scalar fallback
    for (int y = 0; y < vga_canvas::height; y += 2) {
        const float fy = static_cast<float>(y);
        const float dy0_sq = (fy - cy0) * (fy - cy0) + 4.0f;
        const float dy1_sq = (fy - cy1) * (fy - cy1) + 4.0f;
        const float dy2_sq = (fy - cy2) * (fy - cy2) + 4.0f;
        const float dy3_sq = (fy - cy3) * (fy - cy3) + 4.0f;

        uint8_t* row0 = raw + y * vga_canvas::width;
        uint8_t* row1 = row0 + vga_canvas::width;

        for (int x = 0; x < vga_canvas::width; x += 2) {
            const float fx = static_cast<float>(x);
            const float dx0 = fx - cx0;
            const float dx1 = fx - cx1;
            const float dx2 = fx - cx2;
            const float dx3 = fx - cx3;

            const float inv_r0 = 1.0f / std::sqrt(dx0 * dx0 + dy0_sq);
            const float inv_r1 = 1.0f / std::sqrt(dx1 * dx1 + dy1_sq);
            const float inv_r2 = 1.0f / std::sqrt(dx2 * dx2 + dy2_sq);
            const float inv_r3 = 1.0f / std::sqrt(dx3 * dx3 + dy3_sq);

            const float potential = q0_scaled * inv_r0 + q1_scaled * inv_r1 + q2_scaled * inv_r2 + q3_scaled * inv_r3;
            const int lut_idx = static_cast<int>(potential * (1.5f * rad_to_lut)) & 1023;
            const float band = m_sin_lut[lut_idx];

            uint8_t col = 0;
            if (potential > 0.0f) {
                col = static_cast<uint8_t>(101 + std::clamp(static_cast<int>((potential * 3.0f) + band * 15.0f), 0, 98));
            } else {
                col = static_cast<uint8_t>(1 + std::clamp(static_cast<int>((-potential * 3.0f) + band * 15.0f), 0, 98));
            }

            const uint16_t double_pixel = static_cast<uint16_t>(col | (col << 8));
            *reinterpret_cast<uint16_t*>(row0 + x) = double_pixel;
            *reinterpret_cast<uint16_t*>(row1 + x) = double_pixel;
        }
    }
#endif
}

void field_lines_effect::render_retro_grid() {
    // Authentic 1994 FIELD.PAS coordinate grid
    constexpr int step = 10;
    constexpr int mid_x = 160;
    constexpr int mid_y = 100;
    constexpr uint8_t grid_col = 25; // Subtle laboratory grid

    // Dots at grid intersections
    for (int y = mid_y - 8 * step; y <= mid_y + 8 * step; y += step) {
        for (int x = mid_x - 14 * step; x <= mid_x + 14 * step; x += step) {
            m_canvas.put_pixel(x, y, grid_col);
        }
    }

    // Center axes
    for (int x = mid_x - 14 * step; x <= mid_x + 14 * step; ++x) {
        m_canvas.put_pixel(x, mid_y, 45);
    }
    for (int y = mid_y - 8 * step; y <= mid_y + 8 * step; ++y) {
        m_canvas.put_pixel(mid_x, y, 45);
    }

    // Tick marks every 10 pixels
    for (int x = mid_x - 14 * step; x <= mid_x + 14 * step; x += step) {
        m_canvas.put_pixel(x, mid_y - 1, 60);
        m_canvas.put_pixel(x, mid_y + 1, 60);
    }
    for (int y = mid_y - 8 * step; y <= mid_y + 8 * step; y += step) {
        m_canvas.put_pixel(mid_x - 1, y, 60);
        m_canvas.put_pixel(mid_x + 1, y, 60);
    }
}

void field_lines_effect::render_field_lines() {
    constexpr int rays_per_charge = 16;
    for (const auto& c : m_charges) {
        if (c.q > 0.0f) {
            for (int i = 0; i < rays_per_charge; ++i) {
                const float angle = static_cast<float>(i) * (6.2831853f / static_cast<float>(rays_per_charge));
                const float sx = c.pos.x() + std::cos(angle) * (c.radius + 1.0f);
                const float sy = c.pos.y() + std::sin(angle) * (c.radius + 1.0f);
                trace_line(sx, sy, +1.0f);
            }
        }
    }
}

void field_lines_effect::render_charges() {
    for (const auto& c : m_charges) {
        const int cx = static_cast<int>(c.pos.x());
        const int cy = static_cast<int>(c.pos.y());
        const int ir = static_cast<int>(c.radius);

        const uint8_t core_col = (c.q > 0.0f) ? 190 : 80;

        for (int dy = -ir; dy <= ir; ++dy) {
            for (int dx = -ir; dx <= ir; ++dx) {
                if (dx * dx + dy * dy <= ir * ir) {
                    m_canvas.put_pixel(cx + dx, cy + dy, core_col);
                }
            }
        }
        // White center spark
        m_canvas.put_pixel(cx, cy, 255);
    }
}

void field_lines_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    // Orbit charges continuously
    m_charges[0].pos = euler::vec2<float>{160.0f + 70.0f * std::cos(m_time * 0.9f), 100.0f + 45.0f * std::sin(m_time * 0.9f)};
    m_charges[1].pos = euler::vec2<float>{160.0f - 70.0f * std::cos(m_time * 0.9f), 100.0f - 45.0f * std::sin(m_time * 0.9f)};
    m_charges[2].pos = euler::vec2<float>{160.0f + 50.0f * std::sin(m_time * 1.4f), 100.0f + 40.0f * std::cos(m_time * 1.1f)};
    m_charges[3].pos = euler::vec2<float>{160.0f - 50.0f * std::sin(m_time * 1.4f), 100.0f - 40.0f * std::cos(m_time * 1.1f)};

    if (in.pressed(sdlpp::scancode::space)) {
        reset_charges();
    }

    // Toggle display modes: 0 = Heatmap + Lines, 1 = Retro Grid + Lines, 2 = Lines Only
    if (in.pressed(sdlpp::scancode::m)) {
        m_display_mode = (m_display_mode + 1) % 3;
    }

    if (in.pressed(sdlpp::scancode::c)) {
        m_color_theme = (m_color_theme + 1) % 2;
        init_palette();
    }

    if (in.pressed(sdlpp::scancode::r)) {
        reset_charges();
        m_time = 0.0f;
        m_display_mode = 0;
        m_color_theme = 0;
        init_palette();
    }
}

void field_lines_effect::render(const neutrino::rect& viewport) {
    m_canvas.clear(0);

    // 1. Background layer (Heatmap or Retro Grid)
    if (m_display_mode == 0) {
        render_heatmap();
    } else if (m_display_mode == 1) {
        render_retro_grid();
    }

    // 2. Electric Force Field Lines
    render_field_lines();

    // 3. Point Charges
    render_charges();

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
