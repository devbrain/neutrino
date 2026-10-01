#include "bounce_effect.hh"

#include <algorithm>
#include <cmath>

namespace demoscene {

bounce_effect::bounce_effect() {
    on_enter();
}

void bounce_effect::init_palette() {
    // 0: Deep room backdrop
    m_canvas.set_rgb(0, 12, 14, 24);

    // 1..30: Floor tile dark / light ramps
    for (int i = 1; i <= 15; ++i) {
        const float t = static_cast<float>(i) / 15.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(20 + t * 30),
            static_cast<uint8_t>(22 + t * 35),
            static_cast<uint8_t>(30 + t * 45));
    }
    for (int i = 16; i <= 30; ++i) {
        const float t = static_cast<float>(i - 16) / 14.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(40 + t * 50),
            static_cast<uint8_t>(45 + t * 55),
            static_cast<uint8_t>(60 + t * 70));
    }

    // 31..40: Floor shadow ramp (dimmed floor)
    for (int i = 31; i <= 40; ++i) {
        const float t = static_cast<float>(i - 31) / 9.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(8 + t * 12),
            static_cast<uint8_t>(8 + t * 14),
            static_cast<uint8_t>(14 + t * 18));
    }

    // 41..80: Red Rubber Sphere ramp
    for (int i = 41; i <= 80; ++i) {
        const float t = static_cast<float>(i - 41) / 39.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(80 + t * 175),
            static_cast<uint8_t>(t * 70),
            static_cast<uint8_t>(t * 70));
    }

    // 81..120: Cyan Glass Sphere ramp
    for (int i = 81; i <= 120; ++i) {
        const float t = static_cast<float>(i - 81) / 39.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(t * 60),
            static_cast<uint8_t>(90 + t * 165),
            static_cast<uint8_t>(140 + t * 115));
    }

    // 121..160: Golden Yellow Sphere ramp
    for (int i = 121; i <= 160; ++i) {
        const float t = static_cast<float>(i - 121) / 39.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(140 + t * 115),
            static_cast<uint8_t>(110 + t * 145),
            static_cast<uint8_t>(t * 80));
    }

    // 161..200: Emerald Green Sphere ramp
    for (int i = 161; i <= 200; ++i) {
        const float t = static_cast<float>(i - 161) / 39.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(t * 70),
            static_cast<uint8_t>(120 + t * 135),
            static_cast<uint8_t>(40 + t * 90));
    }

    // 201..240: Purple Velvet Sphere ramp
    for (int i = 201; i <= 240; ++i) {
        const float t = static_cast<float>(i - 201) / 39.0f;
        m_canvas.set_rgb(static_cast<uint8_t>(i),
            static_cast<uint8_t>(130 + t * 125),
            static_cast<uint8_t>(t * 60),
            static_cast<uint8_t>(160 + t * 95));
    }
}

void bounce_effect::reset_balls() {
    m_balls.clear();
    // Initialize 5 bouncing spheres with varied radii, initial velocities, and positions
    m_balls.push_back({ 50.0f, 30.0f,  95.0f,  0.0f, 18.0f, 0.0f,  41}); // Red
    m_balls.push_back({120.0f, 60.0f, -80.0f, 40.0f, 15.0f, 0.0f,  81}); // Cyan
    m_balls.push_back({190.0f, 20.0f,  70.0f, -20.0f, 20.0f, 0.0f, 121}); // Gold
    m_balls.push_back({260.0f, 50.0f, -90.0f, 10.0f, 14.0f, 0.0f, 161}); // Emerald
    m_balls.push_back({160.0f, 80.0f,  60.0f, 50.0f, 16.0f, 0.0f, 201}); // Purple
}

void bounce_effect::on_enter() {
    init_palette();
    reset_balls();
    m_gravity = 480.0f;
    m_restitution = 0.82f;
    m_floor_y = 168.0f;
    m_theme = 0;
}

void bounce_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());

    // Interactive Drop New Ball
    if (in.pressed(sdlpp::scancode::space)) {
        const uint8_t colors[] = {41, 81, 121, 161, 201};
        const uint8_t col = colors[m_balls.size() % 5];
        m_balls.push_back({160.0f, 20.0f, (m_balls.size() % 2 == 0 ? 100.0f : -100.0f), 0.0f, 16.0f, 0.0f, col});
    }

    // Gravity controls
    if (in.held(sdlpp::scancode::up))   m_gravity = std::max(100.0f, m_gravity - 150.0f * dt_sec);
    if (in.held(sdlpp::scancode::down)) m_gravity = std::min(900.0f, m_gravity + 150.0f * dt_sec);

    // Elasticity / Restitution controls
    if (in.held(sdlpp::scancode::left))  m_restitution = std::max(0.4f, m_restitution - 0.3f * dt_sec);
    if (in.held(sdlpp::scancode::right)) m_restitution = std::min(0.96f, m_restitution + 0.3f * dt_sec);

    if (in.pressed(sdlpp::scancode::r)) {
        reset_balls();
        m_gravity = 480.0f;
        m_restitution = 0.82f;
    }

    // Step physics for all active spheres
    for (auto& b : m_balls) {
        b.vy += m_gravity * dt_sec;
        b.x += b.vx * dt_sec;
        b.y += b.vy * dt_sec;

        // Ground collision with restitution & squash deformation
        if (b.y + b.radius >= m_floor_y) {
            b.y = m_floor_y - b.radius;
            b.vy = -b.vy * m_restitution;
            b.squash = std::min(0.40f, std::abs(b.vy) / 500.0f);

            // Floor friction
            b.vx *= 0.985f;

            // Keep alive threshold
            if (std::abs(b.vy) < 25.0f && (m_floor_y - b.y - b.radius) < 2.0f) {
                b.vy = -260.0f; // Re-bounce when nearly stopped for continuous demoscene loop
            }
        }

        // Sidewall collision
        if (b.x - b.radius < 10.0f) {
            b.x = 10.0f + b.radius;
            b.vx = -b.vx * m_restitution;
        } else if (b.x + b.radius > 310.0f) {
            b.x = 310.0f - b.radius;
            b.vx = -b.vx * m_restitution;
        }

        // Squash decay back to spherical rest shape
        b.squash = std::max(0.0f, b.squash - 5.0f * dt_sec);
    }
}

void bounce_effect::render_shaded_sphere(int cx, int cy, float rx, float ry, uint8_t base_col) {
    const int irx = static_cast<int>(rx);
    const int iry = static_cast<int>(ry);

    // Directional light vector pointing from top-left front
    constexpr float lx = -0.45f;
    constexpr float ly = -0.55f;
    constexpr float lz = 0.70f;

    for (int dy = -iry; dy <= iry; ++dy) {
        const int py = cy + dy;
        if (py < 0 || py >= vga_canvas::height) continue;

        const float ny = static_cast<float>(dy) / ry;
        const float ny2 = ny * ny;

        for (int dx = -irx; dx <= irx; ++dx) {
            const int px = cx + dx;
            if (px < 0 || px >= vga_canvas::width) continue;

            const float nx = static_cast<float>(dx) / rx;
            const float r2 = nx * nx + ny2;

            if (r2 <= 1.0f) {
                const float nz = std::sqrt(1.0f - r2);

                // Lambertian diffuse dot product: N · L
                const float diffuse = std::max(0.0f, nx * lx + ny * ly + nz * lz);

                // Specular highlight: (N · H)^12
                const float hx = lx;
                const float hy = ly;
                const float hz = lz + 1.0f;
                const float h_len = std::sqrt(hx * hx + hy * hy + hz * hz);
                const float spec_dot = std::max(0.0f, (nx * hx + ny * hy + nz * hz) / h_len);
                const float specular = std::pow(spec_dot, 14.0f);

                const float total = std::clamp(0.20f + 0.65f * diffuse + 0.40f * specular, 0.0f, 1.0f);
                const uint8_t color = static_cast<uint8_t>(base_col + static_cast<int>(total * 38.0f));

                m_canvas.put_pixel_fast(px, py, color);
            }
        }
    }
}

void bounce_effect::render(const neutrino::rect& viewport) {
    m_canvas.clear(0);

    // 1. Draw Checkerboard Floor
    const int floor_start_y = static_cast<int>(m_floor_y - 12.0f);
    for (int y = floor_start_y; y < vga_canvas::height; ++y) {
        const int tile_y = (y - floor_start_y) / 8;
        for (int x = 0; x < vga_canvas::width; ++x) {
            const int tile_x = x / 16;
            const bool tile = ((tile_x ^ tile_y) & 1) == 0;
            const uint8_t floor_col = tile ? 25 : 10;
            m_canvas.put_pixel_fast(x, y, floor_col);
        }
    }

    // 2. Render dynamic projected shadows under each sphere on the floor
    for (const auto& b : m_balls) {
        const float altitude = std::max(0.0f, m_floor_y - (b.y + b.radius));
        const float shadow_rx = b.radius * (1.0f + altitude * 0.005f);
        const float shadow_ry = 5.0f * (1.0f / (1.0f + altitude * 0.02f));

        const int scx = static_cast<int>(b.x);
        const int scy = static_cast<int>(m_floor_y + 2.0f);
        const int isrx = static_cast<int>(shadow_rx);
        const int isry = static_cast<int>(shadow_ry);

        for (int dy = -isry; dy <= isry; ++dy) {
            const int py = scy + dy;
            if (py < floor_start_y || py >= vga_canvas::height) continue;
            for (int dx = -isrx; dx <= isrx; ++dx) {
                const int px = scx + dx;
                if (px < 0 || px >= vga_canvas::width) continue;
                const float ex = static_cast<float>(dx) / shadow_rx;
                const float ey = static_cast<float>(dy) / shadow_ry;
                if (ex * ex + ey * ey <= 1.0f) {
                    m_canvas.put_pixel_fast(px, py, 35); // Shadow color
                }
            }
        }
    }

    // 3. Render 3D Shaded Spheres with squash & stretch deformation
    for (const auto& b : m_balls) {
        // Volume-preserving squash & stretch: rx * ry = R^2
        const float rx = b.radius * (1.0f + b.squash);
        const float ry = b.radius * (1.0f - b.squash * 0.75f);

        render_shaded_sphere(static_cast<int>(b.x), static_cast<int>(b.y), rx, ry, b.base_color_idx);
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
