#include "fire_effect.hh"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace demoscene {

fire_effect::fire_effect() {
    on_enter();
}

void fire_effect::on_enter() {
    m_canvas.set_default_fire_palette();
    m_fire_buffer.fill(0);
    m_wind = 0;
    m_cooling = 2;
    m_emitter_mode = 0;
    m_time = 0.0f;
    m_burst_active = false;
    m_burst_timer = 0.0f;
}

void fire_effect::seed_fuel() {
    std::uniform_int_distribution<int> spark_dist(0, 100);
    std::uniform_int_distribution<int> heat_dist(190, 255);

    constexpr int last_row = vga_canvas::height - 1;
    constexpr int prev_row = vga_canvas::height - 2;

    if (m_burst_active) {
        // Full ignition blast across entire bottom 4 rows
        for (int y = vga_canvas::height - 4; y < vga_canvas::height; ++y) {
            for (int x = 0; x < vga_canvas::width; ++x) {
                m_fire_buffer[static_cast<size_t>(y * vga_canvas::width + x)] = static_cast<uint8_t>(heat_dist(m_rng));
            }
        }
        return;
    }

    switch (m_emitter_mode) {
    case 0: {
        // Full floor inferno: continuous turbulent spark line
        for (int x = 0; x < vga_canvas::width; ++x) {
            const uint8_t heat = (spark_dist(m_rng) < 78) ? static_cast<uint8_t>(heat_dist(m_rng)) : 0;
            m_fire_buffer[static_cast<size_t>(last_row * vga_canvas::width + x)] = heat;
            m_fire_buffer[static_cast<size_t>(prev_row * vga_canvas::width + x)] = heat;
        }
        break;
    }
    case 1: {
        // Center Campfire / Pyre: bell-shaped heat distribution
        for (int x = 0; x < vga_canvas::width; ++x) {
            const float dx = static_cast<float>(x - 160) / 45.0f;
            const float bell = std::exp(-dx * dx);
            uint8_t heat = 0;
            if (spark_dist(m_rng) < static_cast<int>(bell * 95.0f)) {
                heat = static_cast<uint8_t>(180.0f + bell * 75.0f);
            }
            m_fire_buffer[static_cast<size_t>(last_row * vga_canvas::width + x)] = heat;
            m_fire_buffer[static_cast<size_t>(prev_row * vga_canvas::width + x)] = heat;
        }
        break;
    }
    case 2: {
        // Dancing Torches: 3 oscillating fire emitters
        std::fill(m_fire_buffer.begin() + static_cast<size_t>(prev_row * vga_canvas::width), m_fire_buffer.end(), 0);

        const int t1 = static_cast<int>(160.0f + 110.0f * std::sin(m_time * 1.5f));
        const int t2 = static_cast<int>(160.0f + 85.0f * std::cos(m_time * 2.2f));
        const int t3 = static_cast<int>(160.0f - 95.0f * std::sin(m_time * 0.9f));

        const std::array<int, 3> torches{t1, t2, t3};
        for (int center_x : torches) {
            for (int dx = -14; dx <= 14; ++dx) {
                const int px = center_x + dx;
                if (px < 0 || px >= vga_canvas::width) continue;
                const float intensity = 1.0f - std::abs(static_cast<float>(dx)) / 15.0f;
                if (spark_dist(m_rng) < static_cast<int>(intensity * 90.0f)) {
                    const uint8_t heat = static_cast<uint8_t>(180.0f + intensity * 75.0f);
                    m_fire_buffer[static_cast<size_t>(last_row * vga_canvas::width + px)] = heat;
                    m_fire_buffer[static_cast<size_t>(prev_row * vga_canvas::width + px)] = heat;
                }
            }
        }
        break;
    }
    default:
        break;
    }
}

void fire_effect::step_simulation() {
    // Upward convection propagation:
    // Pixel (x, y) samples a weighted 4-pixel neighborhood from rows (y + 1) and (y + 2),
    // displaced horizontally by wind, and cooled by decay.
    for (int y = 0; y < vga_canvas::height - 2; ++y) {
        const int row_y = y * vga_canvas::width;
        const int row_next1 = (y + 1) * vga_canvas::width;
        const int row_next2 = (y + 2) * vga_canvas::width;

        for (int x = 0; x < vga_canvas::width; ++x) {
            // Apply wind deflection to sampling coordinates
            const int sample_center = (x + m_wind + vga_canvas::width) % vga_canvas::width;
            const int sample_left = (sample_center - 1 + vga_canvas::width) % vga_canvas::width;
            const int sample_right = (sample_center + 1) % vga_canvas::width;

            const int p0 = m_fire_buffer[static_cast<size_t>(row_next1 + sample_left)];
            const int p1 = m_fire_buffer[static_cast<size_t>(row_next1 + sample_center)];
            const int p2 = m_fire_buffer[static_cast<size_t>(row_next1 + sample_right)];
            const int p3 = m_fire_buffer[static_cast<size_t>(row_next2 + sample_center)];

            // Heat convection average: (p0 + p1 + p2 + p3) / 4
            const int avg = (p0 + p1 + p2 + p3) >> 2;

            // Stochastic flicker decay
            const int decay = m_cooling + (static_cast<int>(m_rng() & 1));
            const int new_heat = std::max(0, avg - decay);

            m_fire_buffer[static_cast<size_t>(row_y + x)] = static_cast<uint8_t>(new_heat);
        }
    }
}

void fire_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    // Burst ignition timer
    if (m_burst_active) {
        m_burst_timer -= dt_sec;
        if (m_burst_timer <= 0.0f) {
            m_burst_active = false;
        }
    }

    // Interactive controls
    if (in.pressed(sdlpp::scancode::left)) m_wind = std::max(-2, m_wind - 1);
    if (in.pressed(sdlpp::scancode::right)) m_wind = std::min(2, m_wind + 1);

    if (in.pressed(sdlpp::scancode::up)) m_cooling = std::max(1, m_cooling - 1);     // Hotter, taller flames
    if (in.pressed(sdlpp::scancode::down)) m_cooling = std::min(5, m_cooling + 1);   // Colder, shorter flames

    if (in.pressed(sdlpp::scancode::space)) {
        m_burst_active = true;
        m_burst_timer = 0.45f;
    }

    if (in.pressed(sdlpp::scancode::num_1)) m_emitter_mode = 0;
    if (in.pressed(sdlpp::scancode::num_2)) m_emitter_mode = 1;
    if (in.pressed(sdlpp::scancode::num_3)) m_emitter_mode = 2;

    if (in.pressed(sdlpp::scancode::r)) {
        m_wind = 0;
        m_cooling = 2;
        m_emitter_mode = 0;
        m_burst_active = false;
    }

    // Step cellular automata simulation
    m_sim_timer += dt_sec;
    constexpr float sim_step = 1.0f / 60.0f;
    while (m_sim_timer >= sim_step) {
        seed_fuel();
        step_simulation();
        m_sim_timer -= sim_step;
    }
}

void fire_effect::render(const neutrino::rect& viewport) {
    // Copy the simulated 8-bit heat buffer directly to the VGA indexed canvas
    std::memcpy(m_canvas.raw_pixels(), m_fire_buffer.data(), vga_canvas::pixel_count);
    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
