#include "blinking_stars_effect.hh"

#include <algorithm>
#include <cmath>

namespace demoscene {

namespace {

// Authentic 5x5 diffraction spike flare bitmasks from STARS2.PAS
constexpr std::array<std::array<std::array<uint8_t, 5>, 5>, 2> k_bitmasks = {{
    {{
        {0, 0, 1, 0, 0},
        {0, 0, 3, 0, 0},
        {1, 3, 6, 3, 1},
        {0, 0, 3, 0, 0},
        {0, 0, 1, 0, 0}
    }},
    {{
        {0, 0, 6, 0, 0},
        {0, 0, 3, 0, 0},
        {6, 3, 1, 3, 6},
        {0, 0, 3, 0, 0},
        {0, 0, 6, 0, 0}
    }}
}};

} // namespace

blinking_stars_effect::blinking_stars_effect() {
    on_enter();
}

void blinking_stars_effect::on_enter() {
    m_canvas.clear(0);
    init_palette();
    reset_stars();
    m_meteors.clear();
}

void blinking_stars_effect::init_palette() {
    m_canvas.set_rgb_6bit(0, 0, 0, 0); // Void black

    constexpr uint8_t f = 6; // Intensity factor from STARS2.PAS

    if (m_theme == 0) { // Authentic 1995 STARS2.PAS Spectral Rainbow
        for (int i = 1; i <= 10; ++i) {
            const uint8_t c = static_cast<uint8_t>(f * i);

            // Red group (1..30)
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(i), c, 0, 0);
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(21 - i), c, 0, 0);
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(20 + i), 0, 0, 0);

            // Green group (31..60)
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(30 + i), 0, c, 0);
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(51 - i), 0, c, 0);
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(50 + i), 0, 0, 0);

            // Blue group (61..90)
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(60 + i), 0, 0, c);
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(81 - i), 0, 0, c);
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(80 + i), 0, 0, 0);

            // Yellow group (91..120)
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(90 + i), c, c, 0);
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(111 - i), c, c, 0);
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(110 + i), 0, 0, 0);

            // Cyan group (121..150)
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(120 + i), 0, c, c);
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(141 - i), 0, c, c);
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(140 + i), 0, 0, 0);

            // White group (151..180)
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(150 + i), c, c, c);
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(171 - i), c, c, c);
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(170 + i), 0, 0, 0);
        }
    } else if (m_theme == 1) { // Diamond White & Sirius Blue
        for (int i = 1; i <= 10; ++i) {
            const uint8_t c = static_cast<uint8_t>(f * i);
            const uint8_t b = static_cast<uint8_t>(std::min(63, c + 3));

            for (int g = 0; g < 6; ++g) {
                const int base = g * 30;
                m_canvas.set_rgb_6bit(static_cast<uint8_t>(base + i), c, c, b);
                m_canvas.set_rgb_6bit(static_cast<uint8_t>(base + 21 - i), c, c, b);
                m_canvas.set_rgb_6bit(static_cast<uint8_t>(base + 20 + i), 0, 0, 0);
            }
        }
    } else { // Cyberpunk Neon
        for (int i = 1; i <= 10; ++i) {
            const uint8_t c = static_cast<uint8_t>(f * i);

            // Hot Pink
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(i), c, 0, static_cast<uint8_t>(c / 2));
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(21 - i), c, 0, static_cast<uint8_t>(c / 2));

            // Laser Aqua
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(30 + i), 0, c, c);
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(51 - i), 0, c, c);

            // Electric Violet
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(60 + i), static_cast<uint8_t>(c / 2), 0, c);
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(81 - i), static_cast<uint8_t>(c / 2), 0, c);

            // Amber Gold
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(90 + i), c, static_cast<uint8_t>(c * 3 / 4), 0);
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(111 - i), c, static_cast<uint8_t>(c * 3 / 4), 0);

            // Neon Mint
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(120 + i), 0, c, static_cast<uint8_t>(c / 2));
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(141 - i), 0, c, static_cast<uint8_t>(c / 2));

            // Brilliant White
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(150 + i), c, c, c);
            m_canvas.set_rgb_6bit(static_cast<uint8_t>(171 - i), c, c, c);
        }
    }

    // Constellation lines color (index 200..205)
    m_canvas.set_rgb_6bit(200, 10, 15, 25);
    m_canvas.set_rgb_6bit(201, 15, 22, 35);

    // Meteor trail colors (index 210..220)
    for (int i = 0; i <= 10; ++i) {
        const uint8_t c = static_cast<uint8_t>(i * 6);
        m_canvas.set_rgb_6bit(static_cast<uint8_t>(210 + i), c, c, static_cast<uint8_t>(std::min(63, c + 3)));
    }
}

void blinking_stars_effect::reset_stars() {
    m_stars.resize(max_stars);
    std::uniform_int_distribution<int> dist_dur(0, 30);

    for (size_t i = 0; i < max_stars; ++i) {
        m_stars[i] = star{
            .x = 0,
            .y = 0,
            .phase = 0,
            .color_group = 0,
            .duration = dist_dur(m_rng),
            .active = false
        };
    }
}

void blinking_stars_effect::spawn_meteor() {
    std::uniform_real_distribution<float> dist_x(50.0f, 270.0f);
    std::uniform_real_distribution<float> dist_y(0.0f, 60.0f);
    std::uniform_real_distribution<float> dist_speed(180.0f, 260.0f);

    const float angle = 0.75f + (std::uniform_real_distribution<float>(-0.2f, 0.2f)(m_rng));
    const float speed = dist_speed(m_rng);

    m_meteors.push_back(meteor{
        .x = dist_x(m_rng),
        .y = dist_y(m_rng),
        .vx = -std::cos(angle) * speed,
        .vy = std::sin(angle) * speed,
        .life = 1.0f,
        .active = true
    });
}

void blinking_stars_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_nebula_time += dt_sec;

    if (in.pressed(sdlpp::scancode::space)) {
        m_sky_mode = (m_sky_mode + 1) % 3;
    }

    if (in.pressed(sdlpp::scancode::c)) {
        m_theme = (m_theme + 1) % 3;
        init_palette();
    }

    if (in.pressed(sdlpp::scancode::s)) {
        spawn_meteor();
    }

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }

    if (in.pressed(sdlpp::scancode::equals) || in.pressed(sdlpp::scancode::kp_plus)) {
        m_star_count = std::min(max_stars, m_star_count + 20);
    }
    if (in.pressed(sdlpp::scancode::minus) || in.pressed(sdlpp::scancode::kp_minus)) {
        m_star_count = std::max(size_t{40}, m_star_count - 20);
    }

    // Occasional natural shooting stars
    m_meteor_timer += dt_sec;
    if (m_meteor_timer > 4.5f) {
        spawn_meteor();
        m_meteor_timer = 0.0f;
    }

    // Update meteors
    for (auto& m : m_meteors) {
        if (!m.active) continue;
        m.x += m.vx * dt_sec;
        m.y += m.vy * dt_sec;
        m.life -= dt_sec * 1.6f;
        if (m.life <= 0.0f || m.x < 0.0f || m.y > 200.0f) {
            m.active = false;
        }
    }
    std::erase_if(m_meteors, [](const meteor& m) { return !m.active; });

    // Update stars (Authentic STARS2.PAS lifecycle)
    std::uniform_int_distribution<int> dist_x(2, 312);
    std::uniform_int_distribution<int> dist_y(2, 192);
    std::uniform_int_distribution<int> dist_grp(0, 5);
    std::uniform_int_distribution<int> dist_dur(5, 35);

    const size_t count = m_star_count;
    for (size_t i = 0; i < count; ++i) {
        auto& s = m_stars[i];
        if (!s.active) {
            s.duration--;
            if (s.duration < 0) {
                s.active = true;
                s.phase = 0;
                s.color_group = dist_grp(m_rng) * 30;
                s.x = dist_x(m_rng);
                s.y = dist_y(m_rng);
            }
        } else {
            s.phase++;
            if (s.phase >= 20) {
                s.active = false;
                s.duration = dist_dur(m_rng);
            }
        }
    }
}

void blinking_stars_effect::render(const neutrino::rect& viewport) {
    m_canvas.clear(0);
    uint8_t* raw = m_canvas.raw_pixels();

    // Bresenham line helper for constellations and meteors
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

    // Mode 1: Constellation Lines between nearby bright twinkling stars
    if (m_sky_mode == 1) {
        constexpr int max_dist_sq = 45 * 45;
        const size_t count = m_star_count;

        for (size_t i = 0; i < count; ++i) {
            const auto& s1 = m_stars[i];
            if (!s1.active || s1.phase < 5 || s1.phase > 15) continue;

            for (size_t j = i + 1; j < count; ++j) {
                const auto& s2 = m_stars[j];
                if (!s2.active || s2.phase < 5 || s2.phase > 15) continue;

                const int dx = s2.x - s1.x;
                const int dy = s2.y - s1.y;
                const int dist_sq = dx * dx + dy * dy;

                if (dist_sq < max_dist_sq && dist_sq > 16) {
                    draw_line(s1.x + 2, s1.y + 2, s2.x + 2, s2.y + 2, 200);
                }
            }
        }
    }

    // Render active twinkling stars (Authentic bitmask stamp)
    const size_t count = m_star_count;
    for (size_t i = 0; i < count; ++i) {
        const auto& s = m_stars[i];
        if (!s.active) continue;

        const size_t mask_idx = (s.phase > 10) ? 1 : 0;
        const auto& mask = k_bitmasks[mask_idx];

        for (int y = 0; y < 5; ++y) {
            const int dst_y = s.y + y;
            if (dst_y < 0 || dst_y >= vga_canvas::height) continue;
            uint8_t* dst_row = raw + dst_y * vga_canvas::width;

            for (int x = 0; x < 5; ++x) {
                const uint8_t mask_val = mask[y][x];
                if (mask_val > 0) {
                    const int dst_x = s.x + x;
                    if (dst_x >= 0 && dst_x < vga_canvas::width) {
                        // STARS2.PAS formula: bitmask[phase > 10, x, y] + col + phase
                        const int col_idx = mask_val + s.color_group + s.phase;
                        dst_row[dst_x] = static_cast<uint8_t>(col_idx & 0xFF);
                    }
                }
            }
        }
    }

    // Render shooting star streaks
    for (const auto& m : m_meteors) {
        if (!m.active) continue;
        const int tail_len = 16;
        const float dx = -m.vx * 0.08f;
        const float dy = -m.vy * 0.08f;

        const int x0 = static_cast<int>(m.x);
        const int y0 = static_cast<int>(m.y);
        const int x1 = static_cast<int>(m.x - dx);
        const int y1 = static_cast<int>(m.y - dy);

        draw_line(x0, y0, x1, y1, static_cast<uint8_t>(210 + static_cast<int>(m.life * 10.0f)));
        m_canvas.put_pixel(x0, y0, 180); // White-hot meteor head
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
