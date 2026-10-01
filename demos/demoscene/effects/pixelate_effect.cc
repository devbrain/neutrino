#include "pixelate_effect.hh"
#include <onyx_font/bios_font.hh>

#include <cmath>
#include <cstring>
#include <numbers>
#include <algorithm>

namespace demoscene {

namespace {

inline uint8_t clamp_u8(int v) noexcept {
    return static_cast<uint8_t>(std::clamp(v, 0, 255));
}

} // namespace

pixelate_effect::pixelate_effect() {
    init_palette();
}

void pixelate_effect::init_palette() {
    // 0..15: Standard VGA system colors & dark backgrounds
    m_canvas.set_rgb(0, 0, 0, 0);          // Black
    m_canvas.set_rgb(1, 0, 0, 168);        // Blue
    m_canvas.set_rgb(2, 0, 168, 0);        // Green
    m_canvas.set_rgb(3, 0, 168, 168);      // Cyan
    m_canvas.set_rgb(4, 168, 0, 0);        // Red
    m_canvas.set_rgb(5, 168, 0, 168);      // Magenta
    m_canvas.set_rgb(6, 168, 84, 0);       // Brown
    m_canvas.set_rgb(7, 168, 168, 168);    // Light Gray
    m_canvas.set_rgb(8, 84, 84, 84);       // Dark Gray
    m_canvas.set_rgb(9, 84, 84, 252);      // Bright Blue
    m_canvas.set_rgb(10, 84, 252, 84);     // Bright Green
    m_canvas.set_rgb(11, 84, 252, 252);    // Bright Cyan
    m_canvas.set_rgb(12, 252, 84, 84);     // Bright Red
    m_canvas.set_rgb(13, 252, 84, 252);    // Bright Magenta
    m_canvas.set_rgb(14, 252, 252, 84);    // Yellow
    m_canvas.set_rgb(15, 252, 252, 252);   // White

    // 16..79: Copper / Fiery Red-Orange-Gold Ramp (64 colors)
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        uint8_t r, g, b;
        if (t < 0.33f) {
            const float k = t / 0.33f;
            r = static_cast<uint8_t>(k * 220.0f);
            g = static_cast<uint8_t>(k * 30.0f);
            b = 0;
        } else if (t < 0.75f) {
            const float k = (t - 0.33f) / 0.42f;
            r = static_cast<uint8_t>(220.0f + k * 35.0f);
            g = static_cast<uint8_t>(30.0f + k * 180.0f);
            b = static_cast<uint8_t>(k * 40.0f);
        } else {
            const float k = (t - 0.75f) / 0.25f;
            r = 255;
            g = static_cast<uint8_t>(210.0f + k * 45.0f);
            b = static_cast<uint8_t>(40.0f + k * 215.0f);
        }
        m_canvas.set_rgb(static_cast<uint8_t>(16 + i), r, g, b);
    }

    // 80..143: Electric Cyan-Blue-Purple Cosmic Ramp (64 colors)
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        uint8_t r, g, b;
        if (t < 0.5f) {
            const float k = t / 0.5f;
            r = static_cast<uint8_t>(k * 40.0f);
            g = static_cast<uint8_t>(k * 180.0f);
            b = static_cast<uint8_t>(40.0f + k * 215.0f);
        } else {
            const float k = (t - 0.5f) / 0.5f;
            r = static_cast<uint8_t>(40.0f + k * 215.0f);
            g = static_cast<uint8_t>(180.0f + k * 75.0f);
            b = 255;
        }
        m_canvas.set_rgb(static_cast<uint8_t>(80 + i), r, g, b);
    }

    // 144..207: Emerald Neon Ramp (64 colors)
    for (int i = 0; i < 64; ++i) {
        const float t = static_cast<float>(i) / 63.0f;
        const uint8_t r = static_cast<uint8_t>(t * t * 160.0f);
        const uint8_t g = static_cast<uint8_t>(t * 255.0f);
        const uint8_t b = static_cast<uint8_t>(t * 200.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(144 + i), r, g, b);
    }

    // 208..255: Metallic Chrome / Grayscale Ramp (48 colors)
    for (int i = 0; i < 48; ++i) {
        const float t = static_cast<float>(i) / 47.0f;
        const uint8_t val = static_cast<uint8_t>(t * 255.0f);
        m_canvas.set_rgb(static_cast<uint8_t>(208 + i), val, val, val);
    }
}

void pixelate_effect::on_enter() {
    m_time = 0.0f;
    m_transition_timer = 0.0f;
    m_transitioning = false;
    m_transition_swap_done = false;
    m_block_size = 1.0f;
    m_block_size_y = 1.0f;
}

void pixelate_effect::update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) {
    const float delta = std::chrono::duration<float>(dt).count();
    m_time += delta;

    // Handle user inputs
    if (in.pressed(sdlpp::scancode::space)) {
        if (!m_transitioning) {
            m_transitioning = true;
            m_transition_swap_done = false;
            m_transition_timer = 0.0f;
        }
    }

    if (in.pressed(sdlpp::scancode::m)) {
        m_mode = static_cast<Mode>((static_cast<int>(m_mode) + 1) % static_cast<int>(Mode::Count));
    }

    if (in.pressed(sdlpp::scancode::s)) {
        m_sample_method = static_cast<SampleMethod>((static_cast<int>(m_sample_method) + 1) % static_cast<int>(SampleMethod::Count));
    }

    if (in.pressed(sdlpp::scancode::left)) {
        m_current_scene = (m_current_scene + 3) % 4;
    }
    if (in.pressed(sdlpp::scancode::right)) {
        m_current_scene = (m_current_scene + 1) % 4;
    }

    if (in.held(sdlpp::scancode::up)) {
        m_manual_size = std::min(64.0f, m_manual_size + 20.0f * delta);
    }
    if (in.held(sdlpp::scancode::down)) {
        m_manual_size = std::max(1.0f, m_manual_size - 20.0f * delta);
    }

    // Update Pixelation State based on active Mode
    switch (m_mode) {
    case Mode::AutoTransition: {
        constexpr float hold_time = 3.5f;
        constexpr float ramp_time = 1.2f;

        if (!m_transitioning) {
            m_block_size = 1.0f;
            m_block_size_y = 1.0f;
            m_transition_timer += delta;
            if (m_transition_timer >= hold_time) {
                m_transitioning = true;
                m_transition_swap_done = false;
                m_transition_timer = 0.0f;
            }
        } else {
            m_transition_timer += delta;
            if (m_transition_timer < ramp_time) {
                // Ramping up: 1.0 -> 48.0
                const float t = m_transition_timer / ramp_time;
                m_block_size = 1.0f + 47.0f * (t * t);
            } else if (m_transition_timer < ramp_time * 2.0f) {
                // Midpoint: swap scene if not yet done
                if (!m_transition_swap_done) {
                    m_current_scene = (m_current_scene + 1) % 4;
                    m_transition_swap_done = true;
                }
                // Ramping down: 48.0 -> 1.0
                const float t = (m_transition_timer - ramp_time) / ramp_time;
                const float inv_t = 1.0f - t;
                m_block_size = 1.0f + 47.0f * (inv_t * inv_t);
            } else {
                m_block_size = 1.0f;
                m_transitioning = false;
                m_transition_timer = 0.0f;
            }
            m_block_size_y = m_block_size;
        }
        break;
    }

    case Mode::SineBreathe: {
        const float wave = 0.5f + 0.5f * std::sin(m_time * 2.0f);
        m_block_size = 1.0f + 24.0f * (wave * wave);
        m_block_size_y = m_block_size;
        break;
    }

    case Mode::SteppedPowers: {
        // Discrete power of 2 stepping (1, 2, 4, 8, 16, 32, 64)
        static constexpr float steps[] = {1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f, 32.0f, 16.0f, 8.0f, 4.0f, 2.0f};
        constexpr size_t step_count = sizeof(steps) / sizeof(steps[0]);
        const size_t step_idx = static_cast<size_t>(m_time * 2.0f) % step_count;
        m_block_size = steps[step_idx];
        m_block_size_y = m_block_size;
        break;
    }

    case Mode::AnisotropicDecimation: {
        m_block_size = 1.0f + 28.0f * (0.5f + 0.5f * std::sin(m_time * 1.8f));
        m_block_size_y = 1.0f + 8.0f * (0.5f + 0.5f * std::cos(m_time * 2.4f));
        break;
    }

    default:
        break;
    }
}

void pixelate_effect::render_scene(int scene_id, float time, uint8_t* dst) {
    const int w = vga_canvas::width;
    const int h = vga_canvas::height;

    switch (scene_id) {
    // ------------------------------------------------------------------------
    // Scene 0: Copper Bars & Logo with Twinkling Starfield
    // ------------------------------------------------------------------------
    case 0: {
        // Starfield background
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                // Pseudorandom static hash
                const uint32_t hash = (static_cast<uint32_t>(x) * 12345u) ^ (static_cast<uint32_t>(y) * 67891u);
                if ((hash % 1000u) < 4u) {
                    const float twinkle = 0.5f + 0.5f * std::sin(time * 6.0f + static_cast<float>(hash));
                    dst[y * w + x] = static_cast<uint8_t>(208 + static_cast<int>(twinkle * 45.0f));
                } else {
                    dst[y * w + x] = 0; // Black
                }
            }
        }

        // Copper gradient sine ribbons
        for (int y = 0; y < h; ++y) {
            const float fy = static_cast<float>(y);
            const float wave1 = std::sin(fy * 0.05f + time * 3.0f);
            const float wave2 = std::cos(fy * 0.08f - time * 2.0f);
            const float copper_val = 0.5f + 0.5f * (wave1 * 0.6f + wave2 * 0.4f);
            const uint8_t copper_col = static_cast<uint8_t>(16 + static_cast<int>(copper_val * 60.0f));

            if (copper_val > 0.4f) {
                for (int x = 40; x < 280; ++x) {
                    const float fx = static_cast<float>(x);
                    const float bar_sine = std::sin(fx * 0.03f + time * 4.0f) * 12.0f;
                    if (std::abs(fy - (100.0f + bar_sine)) < 36.0f) {
                        dst[y * w + x] = copper_col;
                    }
                }
            }
        }

        // Render "NEUTRINO" 3D Embossed Banner
        const auto& font = onyx_font::bios_font_8x8();
        constexpr std::string_view title = "NEUTRINO";
        constexpr int scale = 3;
        const int title_px = (w - static_cast<int>(title.size()) * 8 * scale) / 2;
        constexpr int title_py = 88;

        for (size_t ci = 0; ci < title.size(); ++ci) {
            const auto glyph = font.get_glyph(static_cast<uint8_t>(title[ci]));
            const int cx = title_px + static_cast<int>(ci) * 8 * scale;
            for (int gy = 0; gy < 8; ++gy) {
                for (int gx = 0; gx < 8; ++gx) {
                    if (glyph.pixel(static_cast<uint16_t>(gx), static_cast<uint16_t>(gy))) {
                        // Drop shadow
                        for (int sy = 0; sy < scale; ++sy) {
                            for (int sx = 0; sx < scale; ++sx) {
                                const int px = cx + gx * scale + sx + 3;
                                const int py = title_py + gy * scale + sy + 3;
                                if (px >= 0 && px < w && py >= 0 && py < h) {
                                    dst[py * w + px] = 0;
                                }
                            }
                        }
                        // Chrome letters
                        const uint8_t chrome_col = static_cast<uint8_t>(208 + (gy * 5));
                        for (int sy = 0; sy < scale; ++sy) {
                            for (int sx = 0; sx < scale; ++sx) {
                                const int px = cx + gx * scale + sx;
                                const int py = title_py + gy * scale + sy;
                                if (px >= 0 && px < w && py >= 0 && py < h) {
                                    dst[py * w + px] = chrome_col;
                                }
                            }
                        }
                    }
                }
            }
        }
        break;
    }

    // ------------------------------------------------------------------------
    // Scene 1: Polar Psychedelic Plasma / Hypnotic Tunnel
    // ------------------------------------------------------------------------
    case 1: {
        for (int y = 0; y < h; ++y) {
            const float dy = static_cast<float>(y - 100);
            for (int x = 0; x < w; ++x) {
                const float dx = static_cast<float>(x - 160);
                const float r = std::sqrt(dx * dx + dy * dy);
                const float angle = std::atan2(dy, dx);

                const float v1 = std::sin(r * 0.08f - time * 4.0f);
                const float v2 = std::cos(angle * 5.0f + time * 2.5f);
                const float v3 = std::sin((dx + dy) * 0.04f + time * 1.5f);

                const float sum = 0.5f + 0.5f * (v1 * 0.45f + v2 * 0.35f + v3 * 0.20f);
                const uint8_t col = static_cast<uint8_t>(80 + static_cast<int>(sum * 63.0f));
                dst[y * w + x] = col;
            }
        }
        break;
    }

    // ------------------------------------------------------------------------
    // Scene 2: 3D Perspective Checkered Floor with Floating Glass Orb
    // ------------------------------------------------------------------------
    case 2: {
        constexpr int horizon_y = 100;

        // Sky twilight gradient
        for (int y = 0; y < horizon_y; ++y) {
            const float t = static_cast<float>(y) / static_cast<float>(horizon_y);
            const uint8_t sky_col = static_cast<uint8_t>(80 + static_cast<int>(t * 40.0f));
            for (int x = 0; x < w; ++x) {
                dst[y * w + x] = sky_col;
            }
        }

        // Perspective checkered ground floor
        for (int y = horizon_y; y < h; ++y) {
            const float dy = static_cast<float>(y - horizon_y + 1);
            const float z = 160.0f / dy;
            for (int x = 0; x < w; ++x) {
                const float dx = static_cast<float>(x - 160);
                const float u = dx * z * 0.08f;
                const float v = z * 0.25f + time * 3.0f;

                const int iu = static_cast<int>(std::floor(u));
                const int iv = static_cast<int>(std::floor(v));
                const bool check = ((iu ^ iv) & 1) != 0;

                const float fog = std::clamp(1.0f - (z * 0.04f), 0.1f, 1.0f);
                const uint8_t col = check
                    ? static_cast<uint8_t>(144 + static_cast<int>(fog * 55.0f))
                    : static_cast<uint8_t>(16 + static_cast<int>(fog * 45.0f));
                dst[y * w + x] = col;
            }
        }

        // Floating 3D Orb with Shadow and Specular Highlight
        const float orb_cx = 160.0f + 60.0f * std::sin(time * 1.6f);
        const float orb_cy = 90.0f + 25.0f * std::cos(time * 2.2f);
        constexpr float orb_radius = 34.0f;

        // Floor Shadow
        const float shadow_cx = orb_cx;
        const float shadow_cy = 150.0f;
        const float shadow_rx = 40.0f;
        const float shadow_ry = 12.0f;

        for (int y = static_cast<int>(shadow_cy - shadow_ry); y <= static_cast<int>(shadow_cy + shadow_ry); ++y) {
            if (y < horizon_y || y >= h) continue;
            for (int x = static_cast<int>(shadow_cx - shadow_rx); x <= static_cast<int>(shadow_cx + shadow_rx); ++x) {
                if (x < 0 || x >= w) continue;
                const float nx = (static_cast<float>(x) - shadow_cx) / shadow_rx;
                const float ny = (static_cast<float>(y) - shadow_cy) / shadow_ry;
                if (nx * nx + ny * ny <= 1.0f) {
                    dst[y * w + x] = static_cast<uint8_t>(dst[y * w + x] / 2); // Darken shadow
                }
            }
        }

        // 3D Sphere Body
        for (int y = static_cast<int>(orb_cy - orb_radius); y <= static_cast<int>(orb_cy + orb_radius); ++y) {
            if (y < 0 || y >= h) continue;
            for (int x = static_cast<int>(orb_cx - orb_radius); x <= static_cast<int>(orb_cx + orb_radius); ++x) {
                if (x < 0 || x >= w) continue;
                const float dx = (static_cast<float>(x) - orb_cx) / orb_radius;
                const float dy = (static_cast<float>(y) - orb_cy) / orb_radius;
                const float r2 = dx * dx + dy * dy;
                if (r2 <= 1.0f) {
                    const float dz = std::sqrt(1.0f - r2);
                    // Light vector (-0.577, -0.577, 0.577)
                    constexpr float lx = -0.577f;
                    constexpr float ly = -0.577f;
                    constexpr float lz = 0.577f;
                    const float diff = std::max(0.0f, dx * lx + dy * ly + dz * lz);
                    // Specular highlight
                    const float spec = std::pow(diff, 12.0f);

                    const int chrome_val = static_cast<int>(diff * 35.0f + spec * 12.0f);
                    dst[y * w + x] = static_cast<uint8_t>(208 + std::clamp(chrome_val, 0, 47));
                }
            }
        }
        break;
    }

    // ------------------------------------------------------------------------
    // Scene 3: Deep Space Nebula with Pulsing Galactic Core
    // ------------------------------------------------------------------------
    case 3:
    default: {
        for (int y = 0; y < h; ++y) {
            const float dy = static_cast<float>(y - 100);
            for (int x = 0; x < w; ++x) {
                const float dx = static_cast<float>(x - 160);
                const float r = std::sqrt(dx * dx + dy * dy);
                const float angle = std::atan2(dy, dx);

                // Spiral galactic arms
                const float spiral = std::sin(angle * 2.0f - (r * 0.05f) + time * 1.8f);
                const float pulse = 0.5f + 0.5f * std::sin(time * 3.0f);
                const float core = std::max(0.0f, 1.0f - (r / (60.0f + pulse * 15.0f)));

                const float glow = std::clamp(core * 1.5f + spiral * 0.35f, 0.0f, 1.0f);
                const uint8_t col = static_cast<uint8_t>(16 + static_cast<int>(glow * 63.0f));
                dst[y * w + x] = col;
            }
        }
        break;
    }
    }
}

void pixelate_effect::apply_pixelation(const uint8_t* src, uint8_t* dst, float block_size_x, float block_size_y) {
    constexpr int w = vga_canvas::width;
    constexpr int h = vga_canvas::height;

    // Fast-path: 1:1 crisp source copy
    if (block_size_x <= 1.05f && block_size_y <= 1.05f) {
        std::memcpy(dst, src, vga_canvas::pixel_count);
        return;
    }

    const int bx = std::max(1, static_cast<int>(block_size_x));
    const int by = std::max(1, static_cast<int>(block_size_y));

    for (int y0 = 0; y0 < h; y0 += by) {
        const int y1 = std::min(h, y0 + by);
        for (int x0 = 0; x0 < w; x0 += bx) {
            const int x1 = std::min(w, x0 + bx);

            uint8_t col = 0;
            switch (m_sample_method) {
            case SampleMethod::TopLeft:
                // Authentic PIXELATE.PAS: mem[u_vidseg:(j*z)*320 + i*z]
                col = src[y0 * w + x0];
                break;

            case SampleMethod::Center: {
                const int cx = (x0 + x1) / 2;
                const int cy = (y0 + y1) / 2;
                col = src[cy * w + cx];
                break;
            }

            case SampleMethod::Average: {
                uint32_t sum = 0;
                uint32_t count = 0;
                for (int py = y0; py < y1; ++py) {
                    for (int px = x0; px < x1; ++px) {
                        sum += src[py * w + px];
                        ++count;
                    }
                }
                col = (count > 0) ? static_cast<uint8_t>(sum / count) : src[y0 * w + x0];
                break;
            }

            default:
                col = src[y0 * w + x0];
                break;
            }

            // Fill macroblock
            for (int py = y0; py < y1; ++py) {
                std::memset(dst + (py * w + x0), col, static_cast<size_t>(x1 - x0));
            }
        }
    }
}

void pixelate_effect::render(const neutrino::rect& viewport) {
    // 1. Render the high-resolution source scene into m_scene_buffer
    render_scene(m_current_scene, m_time, m_scene_buffer.data());

    // 2. Apply mosaic macroblock decimation to the VGA canvas
    apply_pixelation(m_scene_buffer.data(), m_canvas.raw_pixels(), m_block_size, m_block_size_y);

    // 3. Render HUD overlay on canvas
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

    // Mode name
    std::string_view mode_str = "Auto Transition";
    if (m_mode == Mode::SineBreathe) mode_str = "Sine Breathe";
    else if (m_mode == Mode::SteppedPowers) mode_str = "Stepped 2^n";
    else if (m_mode == Mode::AnisotropicDecimation) mode_str = "Anisotropic";

    // Sample method
    std::string_view sample_str = "Top-Left (1994)";
    if (m_sample_method == SampleMethod::Center) sample_str = "Center";
    else if (m_sample_method == SampleMethod::Average) sample_str = "Average";

    char buf[64];
    std::snprintf(buf, sizeof(buf), "Block: %4.1fx%4.1f | %s | %s", m_block_size, m_block_size_y, mode_str.data(), sample_str.data());
    draw_text(buf, 4, 188, 15);

    // Present VGA Canvas to viewport
    m_canvas.render_to_screen(viewport);
}

} // namespace demoscene
