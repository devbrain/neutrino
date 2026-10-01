#include "multi_sprites_effect.hh"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>
#include <onyx_font/bios_font.hh>

namespace demoscene {

namespace {

constexpr std::array<uint8_t, 64> k_bob_template = {{
    0,0,0,7,6,0,0,0,
    0,7,7,3,3,5,4,0,
    0,7,3,3,3,3,3,0,
    7,3,3,3,3,3,3,2,
    6,3,3,3,3,3,3,1,
    0,5,3,3,3,3,1,0,
    0,4,3,3,3,1,1,0,
    0,0,0,2,1,0,0,0
}};

} // namespace

multi_sprites_effect::multi_sprites_effect()
    : m_background(vga_canvas::pixel_count, 0)
{
    init_tables();
    init_bobs();
    init_background();
    on_enter();
}

void multi_sprites_effect::init_tables() {
    constexpr float pi2 = static_cast<float>(std::numbers::pi * 2.0);
    for (int i = 0; i < 256; ++i) {
        const float t = static_cast<float>(i) / 255.0f;
        m_stab1[i] = static_cast<int>(std::round(std::sin(t * pi2) * 100.0f)) + 100;
        m_stab2[i] = static_cast<int>(std::round(std::cos(t * pi2) * 50.0f)) + 50;
    }
}

void multi_sprites_effect::init_bobs() {
    constexpr std::array<uint8_t, 3> tbl = {8, 16, 32};
    for (size_t b = 0; b < 3; ++b) {
        for (size_t i = 0; i < 64; ++i) {
            const uint8_t val = k_bob_template[i];
            m_bobs[b][i] = (val > 0) ? (val | tbl[b]) : 0;
        }
    }
}

void multi_sprites_effect::init_palette() {
    // 0 = Void black
    m_canvas.set_rgb_6bit(0, 0, 0, 0);

    struct rgb6 { uint8_t r, g, b; };
    std::array<rgb6, 256> pal6{};

    if (m_palette_theme == 0) { // Authentic 1995 RGB Additive Mixing
        for (int i = 0; i <= 7; ++i) {
            const uint8_t c = static_cast<uint8_t>(21 + i * 6); // 21..63

            pal6[i | 8] = {c, 0, 0};                 // Red
            pal6[i | 16] = {0, c, 0};                // Green
            pal6[i | 32] = {0, 0, c};                // Blue

            pal6[i | 8 | 16] = {c, c, 0};            // Red + Green = Yellow
            pal6[i | 8 | 32] = {c, 0, c};            // Red + Blue = Magenta
            pal6[i | 16 | 32] = {0, c, c};           // Green + Blue = Cyan
            pal6[i | 8 | 16 | 32] = {c, c, c};       // Red + Green + Blue = White
        }
    } else if (m_palette_theme == 1) { // Pastel CMY Mode
        for (int i = 0; i <= 7; ++i) {
            const uint8_t c = static_cast<uint8_t>(18 + i * 6);
            pal6[i | 8] = {0, c, c};                 // Cyan
            pal6[i | 16] = {c, 0, c};                // Magenta
            pal6[i | 32] = {c, c, 0};                // Yellow

            pal6[i | 8 | 16] = {c, c, static_cast<uint8_t>(c / 2)};
            pal6[i | 8 | 32] = {static_cast<uint8_t>(c / 2), c, c};
            pal6[i | 16 | 32] = {c, static_cast<uint8_t>(c / 2), c};
            pal6[i | 8 | 16 | 32] = {c, c, c};
        }
    } else { // Cyberpunk Neon
        for (int i = 0; i <= 7; ++i) {
            const uint8_t c = static_cast<uint8_t>(20 + i * 6);
            pal6[i | 8] = {c, 0, static_cast<uint8_t>(c / 2)};     // Hot Pink
            pal6[i | 16] = {0, c, static_cast<uint8_t>(c * 3 / 4)};// Electric Aqua
            pal6[i | 32] = {static_cast<uint8_t>(c / 2), 0, c};    // Neon Purple

            pal6[i | 8 | 16] = {static_cast<uint8_t>(c / 2), c, c};
            pal6[i | 8 | 32] = {c, static_cast<uint8_t>(c / 3), c};
            pal6[i | 16 | 32] = {0, c, c};
            pal6[i | 8 | 16 | 32] = {c, c, c};
        }
    }

    // Overlap with grey translucent backdrop (Bit 7 / index 128..255)
    for (int i = 128; i < 256; ++i) {
        pal6[i].r = static_cast<uint8_t>((pal6[i - 128].r / 3) + 14);
        pal6[i].g = static_cast<uint8_t>((pal6[i - 128].g / 3) + 14);
        pal6[i].b = static_cast<uint8_t>((pal6[i - 128].b / 3) + 14);
    }

    // Set DAC registers
    for (int i = 0; i < 256; ++i) {
        m_canvas.set_rgb_6bit(static_cast<uint8_t>(i), pal6[i].r, pal6[i].g, pal6[i].b);
    }

    // Highlight text color (255)
    m_canvas.set_rgb_6bit(255, 63, 63, 63);
}

void multi_sprites_effect::init_background() {
    std::fill(m_background.begin(), m_background.end(), 0);

    if (m_backdrop_mode == 0) { // Authentic 1995 Split Screen
        for (int y = 0; y < 200; ++y) {
            for (int x = 160; x < 320; ++x) {
                m_background[y * 320 + x] = 128; // Translucent grey
            }
        }

        // Draw authentic retro 1995 text: "Apparently this is possible"
        const auto& font = onyx_font::bios_font_8x8();
        auto write_text = [this, &font](std::string_view txt, int px, int py, uint8_t col) {
            int cur_x = px;
            for (char c : txt) {
                auto glyph = font.get_glyph(static_cast<uint8_t>(c));
                for (uint16_t y = 0; y < glyph.height(); ++y) {
                    for (uint16_t x = 0; x < glyph.width(); ++x) {
                        if (glyph.pixel(x, y)) {
                            const int tx = cur_x + x;
                            const int ty = py + y;
                            if (tx >= 0 && tx < 320 && ty >= 0 && ty < 200) {
                                m_background[ty * 320 + tx] = col;
                            }
                        }
                    }
                }
                cur_x += 8;
            }
        };

        write_text("Apparently", 175, 80, 255);
        write_text("this is", 185, 100, 255);
        write_text("possible", 182, 120, 255);
    } else if (m_backdrop_mode == 1) { // Checkered Translucent Grid
        for (int y = 0; y < 200; ++y) {
            for (int x = 0; x < 320; ++x) {
                if (((x / 32) ^ (y / 25)) & 1) {
                    m_background[y * 320 + x] = 128;
                }
            }
        }
    }
}

void multi_sprites_effect::on_enter() {
    init_palette();
    init_background();

    m_sprites.resize(max_sprites);
    for (size_t i = 0; i < max_sprites; ++i) {
        m_sprites[i] = sprite_state{
            .x = 0,
            .y = 0,
            .idx1 = static_cast<uint8_t>((20 + i * 3) & 255),
            .idx2 = static_cast<uint8_t>((50 - i * 5) & 255),
            .bob_type = static_cast<uint8_t>(i % 3)
        };
    }
    m_active_sprites = 100;
}

void multi_sprites_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float dt_sec = static_cast<float>(dt.count());
    m_time += dt_sec;

    if (in.pressed(sdlpp::scancode::space)) {
        m_formation = (m_formation + 1) % 3;
    }

    if (in.pressed(sdlpp::scancode::b)) {
        m_backdrop_mode = (m_backdrop_mode + 1) % 3;
        init_background();
    }

    if (in.pressed(sdlpp::scancode::c)) {
        m_palette_theme = (m_palette_theme + 1) % 3;
        init_palette();
    }

    if (in.pressed(sdlpp::scancode::r)) {
        on_enter();
    }

    if (in.pressed(sdlpp::scancode::equals) || in.pressed(sdlpp::scancode::kp_plus)) {
        m_active_sprites = std::min(max_sprites, m_active_sprites + 10);
    }
    if (in.pressed(sdlpp::scancode::minus) || in.pressed(sdlpp::scancode::kp_minus)) {
        m_active_sprites = std::max(size_t{20}, m_active_sprites - 10);
    }

    // Step sprite trajectories
    for (size_t i = 0; i < m_active_sprites; ++i) {
        auto& s = m_sprites[i];

        if (m_formation == 0) { // Authentic Lissajous Figure-8 Swarm
            s.x = m_stab1[s.idx1] + m_stab2[s.idx2] - 30;
            s.y = 105 + ((m_stab2[s.idx1] - m_stab1[s.idx2]) >> 1);

            s.idx1 = static_cast<uint8_t>((s.idx1 + 1) & 255);
            s.idx2 = static_cast<uint8_t>((s.idx2 + 2) & 255);
        } else if (m_formation == 1) { // Swirling Gravitational Whirlpool
            const float angle = m_time * 2.0f + static_cast<float>(i) * 0.12f;
            const float r = 30.0f + static_cast<float>(i % 40) * 2.5f;
            s.x = 160 + static_cast<int>(std::cos(angle) * r);
            s.y = 100 + static_cast<int>(std::sin(angle) * (r * 0.65f));
        } else { // Sinusoidal Undulating Wave Ribbon
            const float phase = m_time * 3.0f + static_cast<float>(i) * 0.15f;
            s.x = static_cast<int>((i * 320) / m_active_sprites);
            s.y = 100 + static_cast<int>(std::sin(phase) * 55.0f + std::cos(phase * 1.5f) * 25.0f);
        }
    }
}

void multi_sprites_effect::render(const neutrino::rect& viewport) {
    uint8_t* raw = m_canvas.raw_pixels();

    // 1. Copy backdrop onto canvas
    std::memcpy(raw, m_background.data(), vga_canvas::pixel_count);

    // 2. Draw 100 bouncing transparent sprites with bitwise OR
    for (size_t i = 0; i < m_active_sprites; ++i) {
        const auto& s = m_sprites[i];
        const int sx = s.x;
        const int sy = s.y;
        const auto& bob = m_bobs[s.bob_type];

        // Bounds clipping
        if (sx <= -8 || sx >= 320 || sy <= -8 || sy >= 200) continue;

        const int x_start = std::max(0, -sx);
        const int x_end   = std::min(8, 320 - sx);
        const int y_start = std::max(0, -sy);
        const int y_end   = std::min(8, 200 - sy);

        for (int py = y_start; py < y_end; ++py) {
            const int dst_y = sy + py;
            uint8_t* dst_row = raw + dst_y * 320;
            const uint8_t* src_row = bob.data() + py * 8;

            for (int px = x_start; px < x_end; ++px) {
                const uint8_t pixel = src_row[px];
                if (pixel > 0) {
                    // Bitwise additive transparent combination:
                    // Pixels OR together across channels (8 | 16 | 32 | 128)
                    dst_row[sx + px] |= pixel;
                }
            }
        }
    }

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
