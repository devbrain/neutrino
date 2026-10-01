#include "stretch_scroll_effect.hh"
#include <onyx_font/bios_font.hh>

#include <cmath>
#include <cstring>
#include <numbers>
#include <algorithm>

namespace demoscene {

stretch_scroll_effect::stretch_scroll_effect() {
    init_palette();
    init_tables();

    m_messages = {
        "  NEUTRINO DEMOS   ",
        " EFFECT 40 OF 40   ",
        "  STRETCH & WOBBLE ",
        " BY BAS VAN GAALEN ",
        "  HOLLAND - 1994   ",
        " PORTED TO C++20   ",
        " VGA MODE 13H DEMO ",
        " ACCORDION EFFECT4 ",
        " GREETINGS TO ALL  ",
        " OLD-SKOOL SCENERS ",
        " FUTURE CREW * TRSI",
        " FARBRAUSCH * KEFRE",
        " RAZOR 1911 * CNCD ",
        " BAS'S DEMO ARCHIVE",
        " EULER MATH ENGINE ",
        " PURE RETRO NOSTALG",
        "  ENJOY THE VIBES  "
    };

    // Initialize logo for Accordion Stretcher (EFFECT4.PAS)
    const auto& font = onyx_font::bios_font_8x8();
    std::fill(m_logo_pixels.begin(), m_logo_pixels.end(), 0);

    auto draw_logo_str = [this, &font](std::string_view str, int px, int py, uint8_t color) {
        for (size_t ci = 0; ci < str.size(); ++ci) {
            const auto glyph = font.get_glyph(static_cast<uint8_t>(str[ci]));
            const int cx = px + static_cast<int>(ci) * 8;
            for (int gy = 0; gy < 8; ++gy) {
                for (int gx = 0; gx < 8; ++gx) {
                    if (glyph.pixel(static_cast<uint16_t>(gx), static_cast<uint16_t>(gy))) {
                        const int x = cx + gx;
                        const int y = py + gy;
                        if (x >= 0 && x < logo_w && y >= 0 && y < logo_h) {
                            m_logo_pixels[y * logo_w + x] = color;
                        }
                    }
                }
            }
        }
    };

    // Draw retro badge onto m_logo_pixels
    // Border
    for (int x = 0; x < logo_w; ++x) {
        m_logo_pixels[0 * logo_w + x] = 255;
        m_logo_pixels[1 * logo_w + x] = 254;
        m_logo_pixels[(logo_h - 2) * logo_w + x] = 254;
        m_logo_pixels[(logo_h - 1) * logo_w + x] = 255;
    }
    for (int y = 0; y < logo_h; ++y) {
        m_logo_pixels[y * logo_w + 0] = 255;
        m_logo_pixels[y * logo_w + 1] = 254;
        m_logo_pixels[y * logo_w + (logo_w - 2)] = 254;
        m_logo_pixels[y * logo_w + (logo_w - 1)] = 255;
    }
    draw_logo_str("EFFECT NO. 4", 3, 5, 240);
    draw_logo_str("STRETCHER!", 11, 14, 250);
    draw_logo_str("BVG 1994", 19, 23, 230);
}

void stretch_scroll_effect::init_palette() {
    // 0: Deep space black
    m_canvas.set_rgb(0, 0, 0, 0);

    // 1..31: Background copper / dark navy
    for (int i = 1; i < 32; ++i) {
        const float t = static_cast<float>(i) / 31.0f;
        const uint8_t r = static_cast<uint8_t>(t * 40.0f);
        const uint8_t g = static_cast<uint8_t>(t * 20.0f);
        const uint8_t b = static_cast<uint8_t>(t * 80.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(i), r, g, b);
    }

    // 32..95: STRSCR.PAS text color cycling gradient (64 colors)
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        uint8_t r, g, b;
        if (t < 0.33f) {
            const float k = t / 0.33f;
            r = static_cast<uint8_t>(255.0f * k);
            g = static_cast<uint8_t>(60.0f * k);
            b = static_cast<uint8_t>(30.0f * (1.0f - k));
        } else if (t < 0.66f) {
            const float k = (t - 0.33f) / 0.33f;
            r = 255;
            g = static_cast<uint8_t>(60.0f + 195.0f * k);
            b = static_cast<uint8_t>(200.0f * k);
        } else {
            const float k = (t - 0.66f) / 0.34f;
            r = static_cast<uint8_t>(255.0f * (1.0f - k * 0.5f));
            g = 255;
            b = 255;
        }
        m_canvas.set_rgb(static_cast<uint8_t>(32 + i), r, g, b);
    }

    // 96..191: Cyan to Magenta Neon Gradient (96 colors)
    for (int i = 0; i < 96; ++i) {
        const float t = static_cast<float>(i) / 95.0f;
        const uint8_t r = static_cast<uint8_t>(std::sin(t * std::numbers::pi_v<float>) * 255.0f);
        const uint8_t g = static_cast<uint8_t>((1.0f - t) * 230.0f);
        const uint8_t b = static_cast<uint8_t>(100.0f + t * 155.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(96 + i), r, g, b);
    }

    // 192..255: Metallic Chrome & Gold Ramp (64 colors)
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        const uint8_t r = static_cast<uint8_t>(std::min(255.0f, t * 290.0f));
        const uint8_t g = static_cast<uint8_t>(std::min(255.0f, t * 240.0f));
        const uint8_t b = static_cast<uint8_t>(t * 160.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(192 + i), r, g, b);
    }
}

void stretch_scroll_effect::init_tables() {
    // Authentic STRSCR.PAS sine wobble table: stab[i] = round(sin(pi*i/128) * 20)
    for (int i = 0; i < 256; ++i) {
        m_stab[i] = static_cast<int8_t>(std::round(std::sin(std::numbers::pi_v<float> * static_cast<float>(i) / 128.0f) * 20.0f));
    }
}

void stretch_scroll_effect::on_enter() {
    m_time = 0.0f;
    m_subpixel_scroll = 0.0f;
    m_idx1 = 0;
    m_idx2 = 40;
    m_txt_line_idx = 0;

    for (auto& row : m_bitmap) {
        row.fill(0);
    }
}

void stretch_scroll_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float delta = std::chrono::duration<float>(dt).count();
    m_time += delta;

    if (in.pressed(sdlpp::scancode::space)) {
        m_mode = static_cast<Mode>((static_cast<int>(m_mode) + 1) % static_cast<int>(Mode::Count));
    }

    if (in.pressed(sdlpp::scancode::b)) {
        m_show_backdrop = !m_show_backdrop;
    }

    if (in.pressed(sdlpp::scancode::w)) {
        if (m_wobble_scale < 0.6f) m_wobble_scale = 1.0f;
        else if (m_wobble_scale < 1.4f) m_wobble_scale = 2.0f;
        else if (m_wobble_scale < 2.5f) m_wobble_scale = 0.0f;
        else m_wobble_scale = 0.5f;
    }

    if (in.held(sdlpp::scancode::up)) {
        m_stretch_mult = std::min(2.5f, m_stretch_mult + 1.2f * delta);
    }
    if (in.held(sdlpp::scancode::down)) {
        m_stretch_mult = std::max(0.2f, m_stretch_mult - 1.2f * delta);
    }

    if (in.held(sdlpp::scancode::left)) {
        m_scroll_speed = std::max(10.0f, m_scroll_speed - 25.0f * delta);
    }
    if (in.held(sdlpp::scancode::right)) {
        m_scroll_speed = std::min(120.0f, m_scroll_speed + 25.0f * delta);
    }

    // Advance text rasterization buffer
    update_text_raster(delta);
}

void stretch_scroll_effect::update_text_raster(float delta) {
    const auto& font = onyx_font::bios_font_8x8();
    m_subpixel_scroll += m_scroll_speed * delta;

    while (m_subpixel_scroll >= 1.0f) {
        m_subpixel_scroll -= 1.0f;

        // Shift bitmap buffer 1 row upwards: move(bitmap[1,0], bitmap[0,0], sizeof(bitmap)-160);
        for (int y = 0; y < bitmap_rows - 1; ++y) {
            m_bitmap[y] = m_bitmap[y + 1];
        }
        m_bitmap[bitmap_rows - 1].fill(0);

        // Rasterize next scanline of characters onto the bottom row (m_bitmap[40])
        const auto& line_str = m_messages[m_txt_line_idx];
        const int glyph_row = m_idx1 & 7;

        for (int char_idx = 0; char_idx < 20; ++char_idx) {
            const char c = (char_idx < static_cast<int>(line_str.size())) ? line_str[char_idx] : ' ';
            const auto glyph = font.get_glyph(static_cast<uint8_t>(c));

            for (int i = 0; i < 8; ++i) {
                const int bx = char_idx * 8 + i;
                if (bx < bitmap_cols) {
                    if (glyph.pixel(static_cast<uint16_t>(i), static_cast<uint16_t>(glyph_row))) {
                        // Authentic STRSCR.PAS color formula: 32 + (x + i + idx1) and $3f
                        const uint8_t col = static_cast<uint8_t>(32 + ((char_idx + i + m_idx1) & 0x3f));
                        m_bitmap[bitmap_rows - 1][bx] = col;
                    }
                }
            }
        }

        ++m_idx1;
        m_idx2 = (m_idx2 - 2) & 0xff;

        // Advance to next text line after completing an 8-row glyph
        if ((m_idx1 % 8) == 0) {
            m_txt_line_idx = (m_txt_line_idx + 1) % m_messages.size();
        }
    }
}

void stretch_scroll_effect::render_copper_backdrop(float time) {
    constexpr int w = vga_canvas::width;
    constexpr int h = vga_canvas::height;

    for (int y = 0; y < h; ++y) {
        const float fy = static_cast<float>(y);
        const float copper1 = std::sin(fy * 0.04f + time * 2.0f);
        const float copper2 = std::cos(fy * 0.07f - time * 1.5f);
        const float val = 0.5f + 0.5f * (copper1 * 0.6f + copper2 * 0.4f);

        const uint8_t bg_col = static_cast<uint8_t>(1 + static_cast<int>(val * 28.0f));
        std::memset(m_canvas.raw_pixels() + y * w, bg_col, w);
    }
}

void stretch_scroll_effect::render_stretch_scroll() {
    constexpr int w = vga_canvas::width;
    constexpr int h = vga_canvas::height;

    // Authentic STRSCR.PAS scanline repetition and wobble algorithm
    int add = 12; // Start vertical baseline

    for (int y = 0; y < bitmap_rows; ++y) {
        int offset = s_diffsin[(y + m_idx1 + m_idx2) & 0x3f];
        offset = static_cast<int>(std::round(static_cast<float>(offset) * m_stretch_mult));

        if (offset > 0) {
            add += offset;
            int repy = 0;
            while (repy <= offset && (add + repy) < (h - 20)) {
                const int cur_y = add + repy;

                for (int x = 0; x < bitmap_cols; ++x) {
                    const uint8_t col = m_bitmap[y][x];
                    if (col == 0) continue;

                    // Sine wobble lookup
                    int wobble = m_stab[(m_idx2 + add + x) & 0xff];
                    if (m_mode == Mode::HarmonicWobble) {
                        wobble += static_cast<int>(std::sin(m_time * 4.0f + static_cast<float>(x) * 0.12f) * 6.0f);
                    }
                    const int pos_y = cur_y + static_cast<int>(static_cast<float>(wobble) * m_wobble_scale);

                    if (pos_y >= 0 && pos_y < h) {
                        // Horizontal pixel doubling (160 columns -> 320 canvas pixels)
                        const int px = x * 2;
                        if (px + 1 < w) {
                            m_canvas.put_pixel_fast(px, pos_y, col);
                            m_canvas.put_pixel_fast(px + 1, pos_y, col);
                        }
                    }
                }
                ++repy;
            }
        }
    }
}

void stretch_scroll_effect::render_accordion_stretcher() {
    constexpr int w = vga_canvas::width;
    constexpr int h = vga_canvas::height;
    constexpr int start_x = 109;

    // Center the 102x32 logo around y = 40..71
    constexpr int base_y = 35;
    for (int y = 0; y < logo_h; ++y) {
        for (int x = 0; x < logo_w; ++x) {
            const uint8_t col = m_logo_pixels[y * logo_w + x];
            if (col != 0) {
                m_canvas.put_pixel(start_x + x, base_y + y, col);
            }
        }
    }

    // Dynamic accordion rubber-band scanline stretching (EFFECT4.PAS)
    // In EFFECT4.PAS: J ranges from 199 down to 150, stretching scanline J down to 199.
    const float bounce = 0.5f + 0.5f * std::sin(m_time * 3.0f);
    const int stretch_j = static_cast<int>(base_y + logo_h - 1 - (bounce * 14.0f));
    const int target_bottom = static_cast<int>(130.0f + bounce * 55.0f);

    for (int num = target_bottom; num >= stretch_j; --num) {
        if (num >= 0 && num < h) {
            for (int x = 0; x < logo_w; ++x) {
                const uint8_t col = m_canvas.get_pixel(start_x + x, stretch_j);
                if (col != 0) {
                    m_canvas.put_pixel_fast(start_x + x, num, col);
                }
            }
        }
    }
}

void stretch_scroll_effect::render(const neutrino::rect& viewport) {
    if (m_show_backdrop) {
        render_copper_backdrop(m_time);
    } else {
        m_canvas.clear(0);
    }

    switch (m_mode) {
    case Mode::StretchAndWobble:
    case Mode::HarmonicWobble:
        render_stretch_scroll();
        break;

    case Mode::AccordionStretcher:
        render_accordion_stretcher();
        break;

    case Mode::DualLayerRibbon:
        render_accordion_stretcher();
        render_stretch_scroll();
        break;

    default:
        break;
    }

    // HUD overlay
    const auto& font = onyx_font::bios_font_8x8();
    auto draw_text = [this, &font](std::string_view txt, int px, int py, uint8_t color) {
        for (size_t i = 0; i < txt.size(); ++i) {
            const auto glyph = font.get_glyph(static_cast<uint8_t>(txt[i]));
            for (int gy = 0; gy < 8; ++gy) {
                for (int gx = 0; gx < 8; ++gx) {
                    if (glyph.pixel(static_cast<uint16_t>(gx), static_cast<uint16_t>(gy))) {
                        m_canvas.put_pixel(px + static_cast<int>(i) * 8 + gx, py + gy, color);
                    }
                }
            }
        }
    };

    std::string_view mode_str = "STRSCR Wobble";
    if (m_mode == Mode::AccordionStretcher) mode_str = "EFFECT4 Accordion";
    else if (m_mode == Mode::DualLayerRibbon) mode_str = "Dual Stretcher";
    else if (m_mode == Mode::HarmonicWobble) mode_str = "Harmonic Wobble";

    char buf[64];
    std::snprintf(buf, sizeof(buf), "Mode: %s | Stretch: %3.1fx | Wobble: %3.1fx", mode_str.data(), m_stretch_mult, m_wobble_scale);
    draw_text(buf, 4, 188, 255);

    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
