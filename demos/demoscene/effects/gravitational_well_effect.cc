#include "gravitational_well_effect.hh"

#include <algorithm>
#include <cmath>

namespace demoscene {

namespace {

// Authentic 169 coordinates from Bas van Gaalen's ROT7.PAS (1994)
constexpr std::array<std::array<int, 3>, 169> k_coor_tab = {{
    {-75,-75,1},{-75,-63,2},{-75,-51,4},{-75,-39,6},
    {-75,-27,8},{-75,-15,10},{-75,-3,11},{-75,9,10},{-75,21,9},
    {-75,33,7},{-75,45,5},{-75,57,3},{-75,69,2},{-63,-75,2},
    {-63,-63,4},{-63,-51,7},{-63,-39,11},{-63,-27,15},{-63,-15,19},
    {-63,-3,20},{-63,9,20},{-63,21,17},{-63,33,13},{-63,45,9},
    {-63,57,6},{-63,69,3},{-51,-75,4},{-51,-63,7},{-51,-51,12},
    {-51,-39,19},{-51,-27,26},{-51,-15,32},{-51,-3,35},{-51,9,34},
    {-51,21,30},{-51,33,23},{-51,45,16},{-51,57,10},{-51,69,5},
    {-39,-75,6},{-39,-63,11},{-39,-51,19},{-39,-39,30},{-39,-27,41},
    {-39,-15,50},{-39,-3,54},{-39,9,53},{-39,21,46},{-39,33,35},
    {-39,45,24},{-39,57,15},{-39,69,8},{-27,-75,8},{-27,-63,15},
    {-27,-51,26},{-27,-39,41},{-27,-27,56},{-27,-15,68},{-27,-3,74},
    {-27,9,72},{-27,21,63},{-27,33,48},{-27,45,33},{-27,57,20},
    {-27,69,11},{-15,-75,10},{-15,-63,19},{-15,-51,32},{-15,-39,50},
    {-15,-27,68},{-15,-15,84},{-15,-3,91},{-15,9,88},{-15,21,77},
    {-15,33,59},{-15,45,41},{-15,57,25},{-15,69,14},{-3,-75,11},
    {-3,-63,20},{-3,-51,35},{-3,-39,54},{-3,-27,74},{-3,-15,91},
    {-3,-3,99},{-3,9,96},{-3,21,84},{-3,33,64},{-3,45,44},
    {-3,57,27},{-3,69,15},{9,-75,10},{9,-63,20},{9,-51,34},
    {9,-39,53},{9,-27,72},{9,-15,88},{9,-3,96},{9,9,94},
    {9,21,81},{9,33,63},{9,45,43},{9,57,26},{9,69,14},
    {21,-75,9},{21,-63,17},{21,-51,30},{21,-39,46},{21,-27,63},
    {21,-15,77},{21,-3,84},{21,9,81},{21,21,70},{21,33,54},
    {21,45,37},{21,57,23},{21,69,12},{33,-75,7},{33,-63,13},
    {33,-51,23},{33,-39,35},{33,-27,48},{33,-15,59},{33,-3,64},
    {33,9,63},{33,21,54},{33,33,42},{33,45,29},{33,57,18},
    {33,69,10},{45,-75,5},{45,-63,9},{45,-51,16},{45,-39,24},
    {45,-27,33},{45,-15,41},{45,-3,44},{45,9,43},{45,21,37},
    {45,33,29},{45,45,20},{45,57,12},{45,69,7},{57,-75,3},
    {57,-63,6},{57,-51,10},{57,-39,15},{57,-27,20},{57,-15,25},
    {57,-3,27},{57,9,26},{57,21,23},{57,33,18},{57,45,12},
    {57,57,7},{57,69,4},{69,-75,2},{69,-63,3},{69,-51,5},
    {69,-39,8},{69,-27,11},{69,-15,14},{69,-3,15},{69,9,14},
    {69,21,12},{69,33,10},{69,45,7},{69,57,4},{69,69,2}
}};

} // namespace

gravitational_well_effect::gravitational_well_effect() {
    init_grid();
    init_particles();
    on_enter();
}

void gravitational_well_effect::init_grid() {
    for (size_t i = 0; i < total_grid_points; ++i) {
        m_base_points[i] = euler::vec3<float>{
            static_cast<float>(k_coor_tab[i][0]),
            static_cast<float>(k_coor_tab[i][1]),
            static_cast<float>(k_coor_tab[i][2]) - 40.0f // center around vortex throat
        };
    }
}

void gravitational_well_effect::init_particles() {
    m_particles.resize(particle_count);
    for (size_t i = 0; i < particle_count; ++i) {
        const float r = 12.0f + static_cast<float>(i % 30) * 2.8f + (i * 0.15f);
        const float angle = (static_cast<float>(i) * 2.39996f); // golden angle
        const float speed = 18.0f / std::sqrt(r);               // Keplerian orbital speed
        m_particles[i] = particle{
            .radius = r,
            .angle = angle,
            .speed = speed,
            .z_offset = (i % 7 - 3) * 0.8f,
            .color = static_cast<uint8_t>(40 + (i % 23))
        };
    }
}

void gravitational_well_effect::on_enter() {
    m_canvas.clear(0);
    init_palette();
    m_orientation = euler::quaternion<float>::identity();
    m_angular_velocity = {-0.25f, 0.5f, 0.25f};
    m_well_depth = 1.0f;
    m_zoom = 140.0f;
}

void gravitational_well_effect::init_palette() {
    m_canvas.set_rgb(0, 0, 0, 0); // Cosmic void

    switch (m_theme) {
    case 0: { // Authentic 1994 ROT7.PAS Emerald / Cyan Gradient
        for (int i = 0; i < 64; ++i) {
            // port[$3C9] := I div 3; I; I div 2;
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(i),
                static_cast<uint8_t>(i / 3),
                static_cast<uint8_t>(i),
                static_cast<uint8_t>(i / 2));
        }
        for (int i = 64; i < 256; ++i) {
            const float t = static_cast<float>(i - 64) / 191.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(t * 180.0f),
                static_cast<uint8_t>(180.0f + t * 75.0f),
                static_cast<uint8_t>(120.0f + t * 135.0f));
        }
        break;
    }
    case 1: { // Event Horizon Ultraviolet / Violet Flare
        for (int i = 0; i < 256; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(t * 220.0f),
                static_cast<uint8_t>(t * t * 140.0f),
                static_cast<uint8_t>(40.0f + t * 215.0f));
        }
        break;
    }
    case 2: { // Solar Flare / Accretion Plasma Amber
        for (int i = 0; i < 256; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(std::min(255.0f, t * 320.0f)),
                static_cast<uint8_t>(t * t * 230.0f),
                static_cast<uint8_t>(t * t * t * 180.0f));
        }
        break;
    }
    }
}

void gravitational_well_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    if (in.pressed(sdlpp::scancode::space)) {
        m_mode = (m_mode + 1) % 3;
    }

    if (in.pressed(sdlpp::scancode::c)) {
        m_theme = (m_theme + 1) % 3;
        init_palette();
    }

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }

    if (in.held(sdlpp::scancode::w)) m_zoom = std::min(280.0f, m_zoom + 80.0f * dt_sec);
    if (in.held(sdlpp::scancode::s)) m_zoom = std::max(60.0f, m_zoom - 80.0f * dt_sec);

    if (in.held(sdlpp::scancode::up))   m_well_depth = std::min(2.5f, m_well_depth + 1.2f * dt_sec);
    if (in.held(sdlpp::scancode::down)) m_well_depth = std::max(0.1f, m_well_depth - 1.2f * dt_sec);

    if (in.held(sdlpp::scancode::left))  m_angular_velocity[1] -= 2.0f * dt_sec;
    if (in.held(sdlpp::scancode::right)) m_angular_velocity[1] += 2.0f * dt_sec;

    // Apply 3D quaternion rotation
    const float speed = m_angular_velocity.length();
    if (speed > 1e-4f) {
        const euler::vec3<float> axis = m_angular_velocity.normalized();
        const euler::radian<float> angle_delta = euler::radian<float>(speed * dt_sec);
        const euler::quaternion<float> delta_rot = euler::quaternion<float>::from_axis_angle(axis, angle_delta);
        m_orientation = (delta_rot * m_orientation).normalized();
    }

    // Advance orbiting accretion particles
    for (auto& p : m_particles) {
        p.angle += p.speed * dt_sec * 2.5f;
    }
}

void gravitational_well_effect::render(const neutrino::rect& viewport) {
    m_canvas.clear(0);
    uint8_t* raw = m_canvas.raw_pixels();

    constexpr float camera_z = 280.0f;
    constexpr int zc = 300; // ROT7.PAS camera distance

    // Bresenham line helper for wireframe funnel grid
    auto draw_line = [raw](int x0, int y0, int x1, int y1, uint8_t col) {
        int dx = std::abs(x1 - x0);
        int sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0);
        int sy = y0 < y1 ? 1 : -1;
        int err = dx + dy;

        while (true) {
            if (x0 >= 0 && x0 < vga_canvas::width && y0 >= 0 && y0 < vga_canvas::height) {
                raw[y0 * vga_canvas::width + x0] = col;
            }
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    };

    // Transform and project grid points
    std::array<int, total_grid_points> px{};
    std::array<int, total_grid_points> py{};
    std::array<uint8_t, total_grid_points> col{};
    std::array<bool, total_grid_points> visible{};

    for (size_t i = 0; i < total_grid_points; ++i) {
        euler::vec3<float> pt = m_base_points[i];
        pt[2] *= m_well_depth; // Dynamic gravity well throat depth

        const euler::vec3<float> rot = m_orientation.rotate(pt);

        // ROT7.PAS style perspective calculation:
        // Xp := (Xc*Z - X*Zc) div (Z - Zc)
        const float z = rot.z();
        const float denom = z - static_cast<float>(zc);
        if (std::abs(denom) > 1.0f) {
            const int sx = 160 + static_cast<int>((-rot.x() * static_cast<float>(zc) * (m_zoom / 140.0f)) / denom);
            const int sy = 100 + static_cast<int>((-rot.y() * static_cast<float>(zc) * (m_zoom / 140.0f)) / denom);

            px[i] = sx;
            py[i] = sy;

            // Authentic ROT7 depth coloring: 30 + round(Z / 8)
            const int base_c = 30 + static_cast<int>(z * 0.125f);
            col[i] = static_cast<uint8_t>(std::clamp(base_c, 4, 62));

            visible[i] = (sx >= 0 && sx < vga_canvas::width && sy >= 0 && sy < vga_canvas::height);
        } else {
            visible[i] = false;
        }
    }

    // Mode 0: Authentic 1994 Point Cloud
    if (m_mode == 0) {
        for (size_t i = 0; i < total_grid_points; ++i) {
            if (visible[i]) {
                const int sx = px[i];
                const int sy = py[i];
                m_canvas.put_pixel(sx, sy, col[i]);
                // Highlight vortex throat
                if (k_coor_tab[i][2] > 70) {
                    m_canvas.put_pixel(sx + 1, sy, static_cast<uint8_t>(col[i] + 1));
                    m_canvas.put_pixel(sx, sy + 1, static_cast<uint8_t>(col[i] + 1));
                }
            }
        }
    }
    // Mode 1: 3D Wireframe Spacetime Funnel Mesh
    else if (m_mode == 1) {
        for (int r = 0; r < grid_size; ++r) {
            for (int c = 0; c < grid_size; ++c) {
                const int idx = r * grid_size + c;
                if (!visible[idx]) continue;

                // Horizontal grid lines
                if (c + 1 < grid_size) {
                    const int next_c = r * grid_size + (c + 1);
                    if (visible[next_c]) {
                        draw_line(px[idx], py[idx], px[next_c], py[next_c], col[idx]);
                    }
                }
                // Vertical grid lines
                if (r + 1 < grid_size) {
                    const int next_r = (r + 1) * grid_size + c;
                    if (visible[next_r]) {
                        draw_line(px[idx], py[idx], px[next_r], py[next_r], col[idx]);
                    }
                }

                // Grid nodes
                m_canvas.put_pixel(px[idx], py[idx], static_cast<uint8_t>(col[idx] + 2));
            }
        }
    }
    // Mode 2: Gravitational Funnel + Accretion Disk Vortex
    else {
        // First draw subtle wireframe skeleton
        for (int r = 0; r < grid_size; r += 2) {
            for (int c = 0; c < grid_size; ++c) {
                const int idx = r * grid_size + c;
                if (c + 1 < grid_size && visible[idx] && visible[idx + 1]) {
                    draw_line(px[idx], py[idx], px[idx + 1], py[idx + 1], static_cast<uint8_t>(col[idx] / 2));
                }
            }
        }
        for (int c = 0; c < grid_size; c += 2) {
            for (int r = 0; r < grid_size; ++r) {
                const int idx = r * grid_size + c;
                const int next_r = (r + 1) * grid_size + c;
                if (r + 1 < grid_size && visible[idx] && visible[next_r]) {
                    draw_line(px[idx], py[idx], px[next_r], py[next_r], static_cast<uint8_t>(col[idx] / 2));
                }
            }
        }

        // Draw swirling relativistic particle accretion disk
        for (const auto& p : m_particles) {
            const float px_pos = p.radius * std::cos(p.angle);
            const float py_pos = p.radius * std::sin(p.angle);
            // Height in gravity well: z = -M / (r + eps)
            const float pz_pos = (55.0f - (320.0f / (p.radius + 6.0f))) * m_well_depth + p.z_offset;

            const euler::vec3<float> pt{px_pos, py_pos, pz_pos};
            const euler::vec3<float> rot = m_orientation.rotate(pt);

            const float z = rot.z();
            const float denom = z - static_cast<float>(zc);
            if (std::abs(denom) > 1.0f) {
                const int sx = 160 + static_cast<int>((-rot.x() * static_cast<float>(zc) * (m_zoom / 140.0f)) / denom);
                const int sy = 100 + static_cast<int>((-rot.y() * static_cast<float>(zc) * (m_zoom / 140.0f)) / denom);

                if (sx >= 1 && sx < vga_canvas::width - 1 && sy >= 1 && sy < vga_canvas::height - 1) {
                    const uint8_t c = static_cast<uint8_t>(std::clamp(35 + static_cast<int>(z * 0.15f), 10, 63));
                    m_canvas.put_pixel(sx, sy, c);
                    m_canvas.put_pixel(sx + 1, sy, static_cast<uint8_t>(c / 2));
                    m_canvas.put_pixel(sx, sy + 1, static_cast<uint8_t>(c / 2));
                }
            }
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
