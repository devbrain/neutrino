#include "voxel_effect.hh"

#include <algorithm>
#include <cmath>

namespace demoscene {

voxel_effect::voxel_effect()
    : m_heightmap(map_size * map_size, 0)
{
    on_enter();
}

void voxel_effect::generate_heightmap() {
    // Generate organic landscape using multi-octave sinusoidal synthesis
    for (int y = 0; y < map_size; ++y) {
        for (int x = 0; x < map_size; ++x) {
            const float fx = static_cast<float>(x) / static_cast<float>(map_size);
            const float fy = static_cast<float>(y) / static_cast<float>(map_size);

            float val = 0.0f;
            val += 0.500f * (std::sin(fx * 6.283f * 1.0f) * std::cos(fy * 6.283f * 1.0f));
            val += 0.250f * (std::sin(fx * 6.283f * 2.0f + 1.2f) * std::sin(fy * 6.283f * 2.0f + 0.7f));
            val += 0.125f * (std::cos(fx * 6.283f * 4.0f) * std::sin(fy * 6.283f * 4.0f));
            val += 0.065f * (std::sin((fx + fy) * 6.283f * 8.0f));

            // Normalize from [-0.94, 0.94] to [10, 240]
            const float normalized = (val + 1.0f) * 0.5f;
            m_heightmap[static_cast<size_t>(y * map_size + x)] = static_cast<uint8_t>(
                std::clamp(normalized * 220.0f + 20.0f, 10.0f, 250.0f)
            );
        }
    }
}

void voxel_effect::on_enter() {
    generate_heightmap();

    // Setup 256-color palette:
    // 0 = black
    // 1..40 = Sky gradient (dark blue to twilight haze)
    // 41..120 = Deep valley river blue / lowlands green
    // 121..190 = Rocky mountain hills (terracotta / canyon brown)
    // 191..255 = Mountain peaks / snow
    m_canvas.set_rgb(0, 0, 0, 0);

    for (int i = 1; i <= 40; ++i) {
        const float t = static_cast<float>(i) / 40.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(15 + t * 40),
            static_cast<uint8_t>(20 + t * 60),
            static_cast<uint8_t>(40 + t * 140));
    }

    for (int i = 41; i <= 120; ++i) {
        const float t = static_cast<float>(i - 41) / 79.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(20 + t * 50),
            static_cast<uint8_t>(60 + t * 90),
            static_cast<uint8_t>(30 + t * 30));
    }

    for (int i = 121; i <= 200; ++i) {
        const float t = static_cast<float>(i - 121) / 79.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(110 + t * 70),
            static_cast<uint8_t>(80 + t * 50),
            static_cast<uint8_t>(40 + t * 30));
    }

    for (int i = 201; i <= 255; ++i) {
        const float t = static_cast<float>(i - 201) / 54.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(180 + t * 75),
            static_cast<uint8_t>(180 + t * 75),
            static_cast<uint8_t>(190 + t * 65));
    }
}

float voxel_effect::sample_height(float x, float y) const noexcept {
    // Wrap to [0, map_size)
    float wx = std::fmod(x, static_cast<float>(map_size));
    if (wx < 0.0f) wx += static_cast<float>(map_size);
    float wy = std::fmod(y, static_cast<float>(map_size));
    if (wy < 0.0f) wy += static_cast<float>(map_size);

    const int x0 = static_cast<int>(wx);
    const int y0 = static_cast<int>(wy);
    const int x1 = (x0 + 1) & map_mask;
    const int y1 = (y0 + 1) & map_mask;

    const float tx = wx - static_cast<float>(x0);
    const float ty = wy - static_cast<float>(y0);

    const float h00 = static_cast<float>(m_heightmap[static_cast<size_t>(y0 * map_size + (x0 & map_mask))]);
    const float h10 = static_cast<float>(m_heightmap[static_cast<size_t>(y0 * map_size + x1)]);
    const float h01 = static_cast<float>(m_heightmap[static_cast<size_t>(y1 * map_size + (x0 & map_mask))]);
    const float h11 = static_cast<float>(m_heightmap[static_cast<size_t>(y1 * map_size + x1)]);

    const float h0 = h00 * (1.0f - tx) + h10 * tx;
    const float h1 = h01 * (1.0f - tx) + h11 * tx;
    return h0 * (1.0f - ty) + h1 * ty;
}

void voxel_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());

    // Continuous flight along current heading
    m_cam_x += std::cos(m_cam_angle) * m_speed * dt_sec;
    m_cam_y += std::sin(m_cam_angle) * m_speed * dt_sec;

    // Interactive flight controls
    if (in.held(sdlpp::scancode::up)) m_cam_altitude = std::min(150.0f, m_cam_altitude + 40.0f * dt_sec);
    if (in.held(sdlpp::scancode::down)) m_cam_altitude = std::max(12.0f, m_cam_altitude - 40.0f * dt_sec);
    if (in.held(sdlpp::scancode::left)) m_cam_angle += 1.2f * dt_sec;
    if (in.held(sdlpp::scancode::right)) m_cam_angle -= 1.2f * dt_sec;

    if (in.held(sdlpp::scancode::w)) m_speed = std::min(150.0f, m_speed + 30.0f * dt_sec);
    if (in.held(sdlpp::scancode::s)) m_speed = std::max(0.0f, m_speed - 30.0f * dt_sec);

    if (in.pressed(sdlpp::scancode::r)) {
        m_cam_x = 128.0f;
        m_cam_y = 0.0f;
        m_cam_altitude = 35.0f;
        m_cam_angle = 1.5707963f; // pi / 2 (facing +Y)
        m_speed = 45.0f;
    }
}

void voxel_effect::render(const neutrino::rect& viewport) {
    constexpr int horizon_y = 75;

    // 1. Draw twilight sky gradient down to horizon
    for (int y = 0; y < horizon_y; ++y) {
        const uint8_t sky_color = static_cast<uint8_t>(1 + (y * 39) / horizon_y);
        for (int x = 0; x < vga_canvas::width; ++x) {
            m_canvas.put_pixel_fast(x, y, sky_color);
        }
    }

    // Clear terrain space below horizon to distant haze / water color
    for (int y = horizon_y; y < vga_canvas::height; ++y) {
        for (int x = 0; x < vga_canvas::width; ++x) {
            m_canvas.put_pixel_fast(x, y, 41);
        }
    }

    // 2. 1D Occlusion Buffer: tracks the lowest screen Y (highest mountain point) drawn per column
    // Initialized to the screen bottom (height)
    std::array<int, vga_canvas::width> old_y;
    old_y.fill(vga_canvas::height);

    // 3. Camera state & orientation
    const float terrain_under_cam = sample_height(m_cam_x, m_cam_y);
    const float cam_h = terrain_under_cam + m_cam_altitude;

    // Camera forward and right vectors (Euler direction)
    const float fwd_x = std::cos(m_cam_angle);
    const float fwd_y = std::sin(m_cam_angle);
    // Right vector is perpendicular to forward: (sin, -cos)
    const float right_x = std::sin(m_cam_angle);
    const float right_y = -std::cos(m_cam_angle);

    // 4. Front-to-back raymarching: depth min_depth to max_depth
    constexpr int min_depth = 6;
    constexpr int max_depth = 140;
    constexpr float fov_scale = 1.0f;
    constexpr float height_scale = 80.0f;

    for (int d = min_depth; d < max_depth; ++d) {
        const float depth = static_cast<float>(d);
        const float inv_depth = height_scale / depth;

        // View frustum slice endpoints at this depth
        const float half_w = depth * fov_scale;
        const float left_x = m_cam_x + depth * fwd_x - half_w * right_x;
        const float left_y = m_cam_y + depth * fwd_y - half_w * right_y;
        const float right_pos_x = m_cam_x + depth * fwd_x + half_w * right_x;
        const float right_pos_y = m_cam_y + depth * fwd_y + half_w * right_y;

        const float step_x = (right_pos_x - left_x) / static_cast<float>(vga_canvas::width);
        const float step_y = (right_pos_y - left_y) / static_cast<float>(vga_canvas::width);

        float cur_x = left_x;
        float cur_y = left_y;

        // Depth-based fog shade (distant terrain blends into haze)
        const int fog_shade = (max_depth - d) * 15 / max_depth;

        for (int x = 0; x < vga_canvas::width; ++x) {
            const int map_x = static_cast<int>(std::floor(cur_x)) & map_mask;
            const int map_y = static_cast<int>(std::floor(cur_y)) & map_mask;
            const uint8_t height_val = m_heightmap[static_cast<size_t>(map_y * map_size + map_x)];

            // Perspective projection of height
            // When terrain is below camera (cam_h > height_val), proj_y is below horizon (> horizon_y)
            // When terrain is above camera (height_val > cam_h), proj_y is above horizon (< horizon_y)
            const int proj_y = horizon_y + static_cast<int>((cam_h - static_cast<float>(height_val)) * inv_depth);

            // Front-to-back occlusion check: only draw if taller than closer terrain
            if (proj_y < old_y[static_cast<size_t>(x)]) {
                const int y_start = std::max(0, proj_y);
                const int y_end = std::min(vga_canvas::height, old_y[static_cast<size_t>(x)]);

                if (y_start < y_end) {
                    const uint8_t color_idx = static_cast<uint8_t>(
                        std::clamp(static_cast<int>(height_val) + fog_shade, 41, 255)
                    );

                    for (int y = y_start; y < y_end; ++y) {
                        m_canvas.put_pixel_fast(x, y, color_idx);
                    }
                }
                old_y[static_cast<size_t>(x)] = y_start;
            }

            cur_x += step_x;
            cur_y += step_y;
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
