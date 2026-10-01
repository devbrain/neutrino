#include "fractal_landscape_effect.hh"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>

namespace demoscene {

fractal_landscape_effect::fractal_landscape_effect() {
    init_palette();
    generate_terrain();
    on_enter();
}

uint32_t fractal_landscape_effect::rand_u32() {
    m_rng_state ^= m_rng_state << 13;
    m_rng_state ^= m_rng_state >> 17;
    m_rng_state ^= m_rng_state << 5;
    return m_rng_state;
}

float fractal_landscape_effect::rand_f() {
    return static_cast<float>(rand_u32() & 0xFFFF) / 65535.0f;
}

void fractal_landscape_effect::init_palette() {
    // 0..31: Sky Horizon Gradient (Navy to pale atmospheric haze)
    for (int i = 0; i < 32; ++i) {
        const float t = static_cast<float>(i) / 31.0f;
        const uint8_t r = static_cast<uint8_t>(20.0f + t * 90.0f);
        const uint8_t g = static_cast<uint8_t>(35.0f + t * 110.0f);
        const uint8_t b = static_cast<uint8_t>(80.0f + t * 140.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
    }

    // 32..63: Ocean / Water (Deep Azure to Turquoise)
    for (int i = 0; i < 32; ++i) {
        const float t = static_cast<float>(i) / 31.0f;
        const uint8_t r = static_cast<uint8_t>(t * 30.0f);
        const uint8_t g = static_cast<uint8_t>(40.0f + t * 120.0f);
        const uint8_t b = static_cast<uint8_t>(100.0f + t * 140.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(32 + i), r, g, b);
    }

    // 64..79: Sandy Shoreline Beach
    for (int i = 0; i < 16; ++i) {
        const float t = static_cast<float>(i) / 15.0f;
        const uint8_t r = static_cast<uint8_t>(190.0f + t * 50.0f);
        const uint8_t g = static_cast<uint8_t>(170.0f + t * 40.0f);
        const uint8_t b = static_cast<uint8_t>(90.0f + t * 30.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(64 + i), r, g, b);
    }

    // 80..143: Green Valleys & Forests (64 colors)
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        const uint8_t r = static_cast<uint8_t>(20.0f + t * 60.0f);
        const uint8_t g = static_cast<uint8_t>(80.0f + t * 140.0f);
        const uint8_t b = static_cast<uint8_t>(25.0f + t * 40.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(80 + i), r, g, b);
    }

    // 144..191: Rocky Mountain Crags (48 colors)
    for (int i = 0; i < 48; ++i) {
        const float t = static_cast<float>(i) / 47.0f;
        const uint8_t r = static_cast<uint8_t>(80.0f + t * 85.0f);
        const uint8_t g = static_cast<uint8_t>(75.0f + t * 75.0f);
        const uint8_t b = static_cast<uint8_t>(70.0f + t * 70.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(144 + i), r, g, b);
    }

    // 192..255: Snow-Capped Glaciers (64 colors)
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        const uint8_t val = static_cast<uint8_t>(190.0f + t * 65.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(192 + i), val, val, val);
    }
}

void fractal_landscape_effect::generate_terrain() {
    m_heightmap.fill(0);

    // Seed 4 corners for toroidal continuity
    const float initial_corner = rand_f() * 80.0f + 20.0f;
    m_heightmap[0] = static_cast<uint8_t>(initial_corner);
    m_heightmap[map_size - 1] = static_cast<uint8_t>(initial_corner);
    m_heightmap[(map_size - 1) * map_size] = static_cast<uint8_t>(initial_corner);
    m_heightmap[(map_size - 1) * map_size + (map_size - 1)] = static_cast<uint8_t>(initial_corner);

    // Diamond-Square toroidal fractal generation
    float roughness = 70.0f;
    for (int step = map_size; step > 1; step /= 2) {
        const int half = step / 2;

        // Diamond step
        for (int y = half; y < map_size; y += step) {
            for (int x = half; x < map_size; x += step) {
                const int a = m_heightmap[((y - half) & map_mask) * map_size + ((x - half) & map_mask)];
                const int b = m_heightmap[((y - half) & map_mask) * map_size + ((x + half) & map_mask)];
                const int c = m_heightmap[((y + half) & map_mask) * map_size + ((x - half) & map_mask)];
                const int d = m_heightmap[((y + half) & map_mask) * map_size + ((x + half) & map_mask)];
                const float avg = static_cast<float>(a + b + c + d) * 0.25f;
                const float val = avg + (rand_f() - 0.5f) * roughness;
                m_heightmap[y * map_size + x] = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
            }
        }

        // Square step
        for (int y = 0; y < map_size; y += half) {
            for (int x = (y + half) % step; x < map_size; x += step) {
                const int a = m_heightmap[((y - half) & map_mask) * map_size + x];
                const int b = m_heightmap[((y + half) & map_mask) * map_size + x];
                const int c = m_heightmap[y * map_size + ((x - half) & map_mask)];
                const int d = m_heightmap[y * map_size + ((x + half) & map_mask)];
                const float avg = static_cast<float>(a + b + c + d) * 0.25f;
                const float val = avg + (rand_f() - 0.5f) * roughness;
                m_heightmap[y * map_size + x] = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
            }
        }

        roughness *= 0.53f; // Fractal decay
    }
}

void fractal_landscape_effect::on_enter() {
    m_cam_x = 0.0f;
    m_cam_y = 0.0f;
    m_cam_z = 95.0f;
    m_cam_yaw = 0.0f;
    m_cam_pitch = 0.0f;
    m_speed = 35.0f;
    m_time = 0.0f;
}

void fractal_landscape_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    if (in.pressed(sdlpp::scancode::space)) {
        m_rng_state += 99991;
        generate_terrain();
    }

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }

    if (in.held(sdlpp::scancode::left))  m_cam_yaw -= 1.4f * dt_sec;
    if (in.held(sdlpp::scancode::right)) m_cam_yaw += 1.4f * dt_sec;
    if (in.held(sdlpp::scancode::up))    m_cam_pitch = std::min(45.0f, m_cam_pitch + 40.0f * dt_sec);
    if (in.held(sdlpp::scancode::down))  m_cam_pitch = std::max(-35.0f, m_cam_pitch - 40.0f * dt_sec);

    if (in.held(sdlpp::scancode::w)) m_cam_z = std::min(220.0f, m_cam_z + 45.0f * dt_sec);
    if (in.held(sdlpp::scancode::s)) m_cam_z = std::max(20.0f, m_cam_z - 45.0f * dt_sec);

    if (in.held(sdlpp::scancode::equals) || in.held(sdlpp::scancode::kp_plus)) {
        m_speed = std::min(90.0f, m_speed + 25.0f * dt_sec);
    }
    if (in.held(sdlpp::scancode::minus) || in.held(sdlpp::scancode::kp_minus)) {
        m_speed = std::max(5.0f, m_speed - 25.0f * dt_sec);
    }

    // Advance flight position along yaw direction
    m_cam_x += std::sin(m_cam_yaw) * m_speed * dt_sec;
    m_cam_y += std::cos(m_cam_yaw) * m_speed * dt_sec;

    // Minimum camera altitude above terrain
    const int u = static_cast<int>(m_cam_x) & map_mask;
    const int v = static_cast<int>(m_cam_y) & map_mask;
    const float ground = static_cast<float>(m_heightmap[v * map_size + u]);
    if (m_cam_z < ground + 14.0f) {
        m_cam_z = ground + 14.0f;
    }
}

void fractal_landscape_effect::render(const neutrino::rect& viewport) {
    uint8_t* raw = m_canvas.raw_pixels();

    constexpr int w = vga_canvas::width;
    constexpr int h = vga_canvas::height;
    constexpr float fov = 1.15f; // Field of view in radians (~66 degrees)
    const float fov_step = fov / static_cast<float>(w);

    const int horizon_y = 100 + static_cast<int>(m_cam_pitch);

    // Raymarch column projection
    for (int x = 0; x < w; ++x) {
        const float ray_angle = m_cam_yaw + (static_cast<float>(x) - 160.0f) * fov_step;
        const float sin_a = std::sin(ray_angle);
        const float cos_a = std::cos(ray_angle);

        int old_y = h;
        float d = 8.0f;

        while (d < 260.0f) {
            const float px = m_cam_x + sin_a * d;
            const float py = m_cam_y + cos_a * d;

            const int u = static_cast<int>(px) & map_mask;
            const int v = static_cast<int>(py) & map_mask;
            const uint8_t height = m_heightmap[v * map_size + u];

            // Perspective height projection
            const int screen_y = horizon_y - static_cast<int>((static_cast<float>(height) - m_cam_z) * 115.0f / d);

            if (screen_y < old_y) {
                const int draw_top = std::max(0, screen_y);
                const int draw_bot = std::min(h, old_y);

                // Determine altitude color with distance fog
                uint8_t col = 80;
                if (height < 35) {
                    col = static_cast<uint8_t>(32 + (height % 32)); // Water
                } else if (height < 50) {
                    col = static_cast<uint8_t>(64 + ((height - 35) % 16)); // Sand
                } else if (height < 115) {
                    col = static_cast<uint8_t>(80 + ((height - 50) % 64)); // Grass
                } else if (height < 165) {
                    col = static_cast<uint8_t>(144 + ((height - 115) % 48)); // Rock
                } else {
                    col = static_cast<uint8_t>(192 + ((height - 165) % 64)); // Snow
                }

                // Fog fade for distant terrain
                if (d > 140.0f) {
                    const float fog = (d - 140.0f) / 120.0f;
                    col = static_cast<uint8_t>(col * (1.0f - fog * 0.6f) + 15.0f * fog);
                }

                for (int y = draw_top; y < draw_bot; ++y) {
                    raw[y * w + x] = col;
                }

                old_y = screen_y;
                if (old_y <= 0) break; // Column fully occluded
            }

            d += 1.0f + d * 0.016f; // Progressive distance step
        }

        // Draw sky above the highest terrain peak
        for (int y = 0; y < old_y && y < h; ++y) {
            const int sky_idx = std::clamp((y * 31) / std::max(1, horizon_y), 0, 31);
            raw[y * w + x] = static_cast<uint8_t>(sky_idx);
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
