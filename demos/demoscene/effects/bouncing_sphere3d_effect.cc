#include "bouncing_sphere3d_effect.hh"

#include <algorithm>
#include <cmath>

namespace demoscene {

namespace {

// Authentic parabolic bounce table from ROT10.PAS (1994)
constexpr std::array<uint8_t, 256> k_ptab = {{
    123,121,119,117,115,114,112,110,108,106,104,103,101,99,97,96,94,92,91,
    89,87,86,84,82,81,79,78,76,75,73,72,70,69,67,66,64,63,62,60,59,58,56,
    55,54,52,51,50,49,48,46,45,44,43,42,41,39,38,37,36,35,34,33,32,31,30,
    29,28,27,26,26,25,24,23,22,21,21,20,19,18,17,17,16,15,15,14,13,13,12,
    12,11,10,10,9,9,8,8,7,7,6,6,5,5,5,4,4,4,3,3,3,2,2,2,2,1,1,1,1,1,1,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,5,5,6,6,
    7,7,7,8,8,9,9,10,11,11,12,12,13,14,14,15,16,16,17,18,19,19,20,21,22,
    23,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,
    46,47,48,49,51,52,53,54,56,57,58,60,61,62,64,65,67,68,69,71,72,74,75,
    77,78,80,82,83,85,86,88,90,91,93,95,96,98,100,102,103,105,107,109,111,
    113,114,116,118,120,122,124,126
}};

// Authentic 100 spherical coordinates from ROT10.PAS
constexpr std::array<std::array<int, 3>, 100> k_ctab = {{
    {-18,24,2},{14,-19,19},{23,14,-13},{-1,22,-20},{-3,1,30},{-1,5,30},
    {-11,-27,-4},{-1,0,-30},{-12,-11,25},{-18,-13,20},{-3,12,27},
    {-27,6,-13},{-30,-1,1},{-6,-9,-28},{4,-28,11},{2,22,-20},{-5,1,-30},
    {2,1,30},{-7,21,21},{-7,18,-23},{17,-22,-11},{-10,5,28},{0,-1,30},
    {11,-25,-13},{-6,-28,-10},{13,12,-24},{0,0,-30},{-20,21,8},{-3,-30,-4},
    {16,7,-24},{13,-4,-27},{4,-9,-28},{-10,-1,-28},{-19,-22,-8},{7,-6,29},
    {-16,-22,-13},{23,6,-18},{22,-7,-19},{-5,3,-30},{-3,5,-29},{12,0,28},
    {-6,13,-26},{24,-16,-8},{-7,23,18},{-10,28,-5},{21,20,8},{19,-5,23},
    {0,10,-28},{23,13,-14},{4,-6,29},{19,12,20},{8,-17,-23},{17,21,13},
    {-16,3,25},{-2,4,30},{-24,17,3},{-2,-1,-30},{-9,-8,27},{-10,4,-28},
    {10,-19,21},{3,22,-20},{-6,1,29},{-22,-21,3},{0,-1,-30},{30,1,4},
    {-29,7,-1},{-6,23,-18},{-10,-28,3},{-3,10,-28},{16,-23,-10},
    {-8,23,-17},{-6,3,29},{2,-19,24},{-13,14,-23},{13,-26,9},{-17,21,-12},
    {8,2,29},{16,-13,22},{9,9,27},{7,-15,25},{-25,16,-2},{-1,-3,-30},
    {18,0,-24},{12,-3,27},{3,3,-30},{-22,-16,-13},{-5,-5,29},{21,-14,-16},
    {3,21,21},{21,-20,-8},{27,6,12},{-13,-13,-23},{1,11,-28},{25,-14,-9},
    {3,1,-30},{-2,-3,-30},{1,2,30},{8,20,21},{-20,22,6},{11,13,25}
}};

} // namespace

bouncing_sphere3d_effect::bouncing_sphere3d_effect() {
    init_points();
    on_enter();
}

void bouncing_sphere3d_effect::init_points() {
    for (size_t i = 0; i < dots_count; ++i) {
        m_dots[i] = euler::vec3<float>{
            static_cast<float>(k_ctab[i][0]),
            static_cast<float>(k_ctab[i][1]),
            static_cast<float>(k_ctab[i][2])
        };
    }
}

void bouncing_sphere3d_effect::on_enter() {
    m_canvas.clear(0);
    init_palette();
    m_obj_x = 160.0f;
    m_x_vel = 75.0f;
    m_pc = 128.0f;
    m_bounce_speed = 1.0f;
    m_orientation = euler::quaternion<float>::identity();
    m_angular_velocity = {0.8f, 1.2f, -0.6f};
    m_time = 0.0f;
}

void bouncing_sphere3d_effect::init_palette() {
    m_canvas.set_rgb_6bit(0, 0, 0, 0); // Void black

    switch (m_theme) {
    case 0: { // Authentic ROT10.PAS Palette
        // for i := 1 to 64 do setpal(i, 10+i div 3, 10+i div 2, i);
        for (int i = 1; i <= 64; ++i) {
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(i),
                static_cast<uint8_t>(10 + i / 3),
                static_cast<uint8_t>(10 + i / 2),
                static_cast<uint8_t>(i));
        }
        for (int i = 65; i <= 255; ++i) {
            const float t = static_cast<float>(i - 65) / 190.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(120.0f + t * 135.0f),
                static_cast<uint8_t>(180.0f + t * 75.0f),
                255);
        }
        break;
    }
    case 1: { // Emerald Matrix Neon
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(t * t * 60.0f),
                static_cast<uint8_t>(30.0f + t * 225.0f),
                static_cast<uint8_t>(t * 80.0f));
        }
        break;
    }
    case 2: { // Magma Amber Fire
        for (int i = 1; i <= 255; ++i) {
            const float t = static_cast<float>(i) / 255.0f;
            m_canvas.set_rgb(static_cast<uint8_t>(i),
                static_cast<uint8_t>(std::min(255.0f, t * 300.0f)),
                static_cast<uint8_t>(t * t * 210.0f),
                static_cast<uint8_t>(t * t * t * 70.0f));
        }
        break;
    }
    }
}

void bouncing_sphere3d_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
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

    if (in.held(sdlpp::scancode::equals) || in.held(sdlpp::scancode::kp_plus)) {
        m_bounce_speed = std::min(2.5f, m_bounce_speed + 0.8f * dt_sec);
    }
    if (in.held(sdlpp::scancode::minus) || in.held(sdlpp::scancode::kp_minus)) {
        m_bounce_speed = std::max(0.4f, m_bounce_speed - 0.8f * dt_sec);
    }

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

    // Kinematics: wall-to-wall horizontal bounce
    m_obj_x += m_x_vel * dt_sec * m_bounce_speed;
    if (m_obj_x < 45.0f) {
        m_obj_x = 45.0f;
        m_x_vel = -m_x_vel;
    } else if (m_obj_x > 275.0f) {
        m_obj_x = 275.0f;
        m_x_vel = -m_x_vel;
    }

    // Advance parabolic bounce index
    m_pc += dt_sec * 120.0f * m_bounce_speed;
    if (m_pc >= 256.0f) {
        m_pc = std::fmod(m_pc, 256.0f);
    }
}

void bouncing_sphere3d_effect::render(const neutrino::rect& viewport) {
    m_canvas.clear(0);
    uint8_t* raw = m_canvas.raw_pixels();

    constexpr float dist = 100.0f;

    // Hoist 3D rotation matrix
    const auto mat = m_orientation.to_matrix3();
    const float r00 = mat(0, 0), r01 = mat(0, 1), r02 = mat(0, 2);
    const float r10 = mat(1, 0), r11 = mat(1, 1), r12 = mat(1, 2);
    const float r20 = mat(2, 0), r21 = mat(2, 1), r22 = mat(2, 2);

    const int bounce_y = 35 + k_ptab[static_cast<size_t>(m_pc) & 255];
    const float center_x = m_obj_x;

    // Render floor grid shadow
    const int shadow_y = 175;
    const int shadow_x = static_cast<int>(center_x);
    const int shadow_r = std::max(6, 32 - (175 - bounce_y) / 4);

    for (int dy = -4; dy <= 4; ++dy) {
        for (int dx = -shadow_r; dx <= shadow_r; ++dx) {
            if ((dx * dx) / static_cast<float>(shadow_r * shadow_r) + (dy * dy) / 16.0f <= 1.0f) {
                const int py = shadow_y + dy;
                const int px = shadow_x + dx;
                if (px >= 0 && px < vga_canvas::width && py >= 0 && py < vga_canvas::height) {
                    raw[py * vga_canvas::width + px] = 12; // Subtle shadow
                }
            }
        }
    }

    // Mode 0: Authentic 1994 Point Cloud
    if (m_mode == 0) {
        for (size_t n = 0; n < dots_count; ++n) {
            const auto& pt = m_dots[n];
            const float x = pt.x();
            const float y = pt.y();
            const float z = pt.z();

            const float rx = r00 * x + r01 * y + r02 * z;
            const float ry = r10 * x + r11 * y + r12 * z;
            const float rz = r20 * x + r21 * y + r22 * z;

            const float denom = rz - dist;
            if (std::abs(denom) > 1.0f) {
                // ROT10.PAS perspective formula:
                // xp := objx + (-x*dist) div (z-dist)
                // yp := 50 + ptab[pc] + (-y*dist) div (z-dist)
                const int sx = static_cast<int>(center_x + (-rx * dist) / denom);
                const int sy = static_cast<int>(static_cast<float>(bounce_y) + (-ry * dist) / denom);

                if (sx >= 0 && sx < vga_canvas::width && sy >= 0 && sy < vga_canvas::height) {
                    // ROT10 color: z div 2 + 32 (range 0..64)
                    const int col_val = std::clamp(static_cast<int>(rz * 0.5f) + 32, 1, 64);
                    raw[sy * vga_canvas::width + sx] = static_cast<uint8_t>(col_val);

                    if (col_val > 45 && sx + 1 < vga_canvas::width) {
                        raw[sy * vga_canvas::width + sx + 1] = static_cast<uint8_t>(col_val / 2);
                    }
                }
            }
        }
    }
    // Mode 1: Wireframe Rings (Latitude & Longitude)
    else if (m_mode == 1) {
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

        // Draw 6 latitude rings
        constexpr int ring_steps = 24;
        constexpr float radius = 30.0f;

        for (int ring = -2; ring <= 2; ++ring) {
            const float lat_y = (static_cast<float>(ring) / 3.0f) * radius;
            const float lat_r = std::sqrt(std::max(0.0f, radius * radius - lat_y * lat_y));

            int prev_sx = -1, prev_sy = -1;
            for (int s = 0; s <= ring_steps; ++s) {
                const float angle = (static_cast<float>(s) / static_cast<float>(ring_steps)) * 6.283f;
                const float x = lat_r * std::cos(angle);
                const float z = lat_r * std::sin(angle);
                const float y = lat_y;

                const float rx = r00 * x + r01 * y + r02 * z;
                const float ry = r10 * x + r11 * y + r12 * z;
                const float rz = r20 * x + r21 * y + r22 * z;

                const float denom = rz - dist;
                const int sx = static_cast<int>(center_x + (-rx * dist) / denom);
                const int sy = static_cast<int>(static_cast<float>(bounce_y) + (-ry * dist) / denom);

                if (prev_sx >= 0) {
                    const uint8_t col = static_cast<uint8_t>(std::clamp(static_cast<int>(rz * 0.5f) + 36, 10, 64));
                    draw_line(prev_sx, prev_sy, sx, sy, col);
                }
                prev_sx = sx; prev_sy = sy;
            }
        }
    }
    // Mode 2: Multi-Sphere Cascade
    else {
        constexpr std::array<float, 3> x_offsets = {-45.0f, 0.0f, 45.0f};
        constexpr std::array<int, 3> pc_offsets = {0, 64, 128};

        for (size_t s = 0; s < 3; ++s) {
            const int s_bounce_y = 35 + k_ptab[(static_cast<size_t>(m_pc) + pc_offsets[s]) & 255];
            const float s_center_x = std::clamp(center_x + x_offsets[s], 35.0f, 285.0f);

            for (size_t n = 0; n < dots_count; n += 2) {
                const auto& pt = m_dots[n];
                const float x = pt.x() * 0.75f;
                const float y = pt.y() * 0.75f;
                const float z = pt.z() * 0.75f;

                const float rx = r00 * x + r01 * y + r02 * z;
                const float ry = r10 * x + r11 * y + r12 * z;
                const float rz = r20 * x + r21 * y + r22 * z;

                const float denom = rz - dist;
                const int sx = static_cast<int>(s_center_x + (-rx * dist) / denom);
                const int sy = static_cast<int>(static_cast<float>(s_bounce_y) + (-ry * dist) / denom);

                if (sx >= 0 && sx < vga_canvas::width && sy >= 0 && sy < vga_canvas::height) {
                    const int col = std::clamp(static_cast<int>(rz * 0.5f) + 32, 1, 64);
                    raw[sy * vga_canvas::width + sx] = static_cast<uint8_t>(col);
                }
            }
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
