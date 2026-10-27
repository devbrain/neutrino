//
// Neutrino Drawing Tutorial 05: Working with Colors
//
// Demonstrates the colour vocabulary from <neutrino/video/color.hh>, one row per operation:
// 1. mix() / lerp()     -- a gradient between two colours, plus an animated swatch.
// 2. premultiply()      -- straight alpha drawn over a checkerboard vs. premultiplied rgb.
// 3. luminance() / to_grayscale() -- palette colours above their BT.709 grey.
// 4. operator+          -- additive light mixing (red + green + blue), saturating at 255.
// 5. operator*          -- a brightness ramp from x0 to x2.
// 6. color <-> fcolor   -- an animation computed in float channels, converted for drawing,
//                          and the truncation that conversion performs.
//
// Controls: Left/Right pick the FROM colour, Up/Down pick the TO colour, Space pauses the
// animation, Escape quits.
//

#include <neutrino/application.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>
#include <neutrino/video/color.hh>
#include <neutrino/video/draw.hh>

#include <neutrino/entry_point.hh>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>

namespace {

    constexpr int logical_width = 640;
    constexpr int logical_height = 360;
    constexpr int window_width = 1280;
    constexpr int window_height = 720;

    // Every row spans the same horizontal band.
    constexpr int left = 20;
    constexpr int right = 620;
    constexpr int row_height = 40;

    // Named colours from the X11 palette. Note that colors::green is (0, 255, 0) -- X11, not CSS.
    constexpr std::array palette{
        neutrino::colors::crimson,      neutrino::colors::tomato,    neutrino::colors::orange,
        neutrino::colors::gold,         neutrino::colors::lime_green, neutrino::colors::forest_green,
        neutrino::colors::sky_blue,     neutrino::colors::cornflower_blue, neutrino::colors::royal_blue,
        neutrino::colors::orchid,       neutrino::colors::magenta,   neutrino::colors::white,
    };

    class colors_scene final : public neutrino::base_scene {
    public:
        void on_enter() override {
            std::cout << "[Drawing 05] Rows, top to bottom:\n"
                         "  1. mix/lerp: FROM -> TO gradient; the right swatch animates t\n"
                         "  2. premultiply: straight alpha over a checkerboard (top) vs premultiplied rgb (bottom)\n"
                         "  3. to_grayscale: palette colours (top) and their BT.709 grey (bottom)\n"
                         "  4. operator+: R | R+G | R+G+B | G+B | B, then a dim colour added to itself\n"
                         "  5. operator*: FROM scaled by 0.0 .. 2.0\n"
                         "  6. color <-> fcolor: a float-channel pulse; fcolor gray 0.5 -> 127 vs colors::gray 190\n"
                         "Left/Right: FROM colour, Up/Down: TO colour, Space: pause, Escape: quit\n";
        }

        void fixed_update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override {
            if (in.pressed(neutrino::key::escape)) {
                neutrino::pop_scene();
                return;
            }
            if (in.pressed(neutrino::key::right)) m_from = (m_from + 1) % palette.size();
            if (in.pressed(neutrino::key::left))  m_from = (m_from + palette.size() - 1) % palette.size();
            if (in.pressed(neutrino::key::up))    m_to = (m_to + 1) % palette.size();
            if (in.pressed(neutrino::key::down))  m_to = (m_to + palette.size() - 1) % palette.size();
            if (in.pressed(neutrino::key::space)) m_paused = !m_paused;

            if (!m_paused) {
                m_time += dt.count();
            }
        }

        void render() override {
            neutrino::draw_rect_fill(neutrino::rect{0, 0, logical_width, logical_height}, neutrino::color{18, 20, 28});

            draw_mix_row(12);
            draw_premultiply_row(70);
            draw_grayscale_row(128);
            draw_additive_row(186);
            draw_brightness_row(244);
            draw_float_row(302);
        }

    private:
        [[nodiscard]] neutrino::color from() const { return palette[m_from]; }
        [[nodiscard]] neutrino::color to() const { return palette[m_to]; }

        // A 0..1 triangle wave, so the animation sweeps back and forth.
        [[nodiscard]] float wave(float period_sec) const {
            const float phase = std::fmod(m_time / period_sec, 1.0f);
            return phase < 0.5f ? phase * 2.0f : 2.0f - phase * 2.0f;
        }

        static void swatch(int x, int y, int w, int h, const neutrino::color& c) {
            neutrino::draw_rect_fill(neutrino::rect{x, y, w, h}, c);
        }

        // 1. mix() is the member; lerp(a, b, t) is the same thing as a free function.
        void draw_mix_row(int y) const {
            constexpr int steps = 24;
            constexpr int strip_w = 480;
            constexpr int cell = strip_w / steps;
            for (int i = 0; i < steps; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(steps - 1);
                swatch(left + i * cell, y, cell, row_height, from().mix(to(), t));
            }

            const float t = wave(4.0f);
            const int marker_x = left + static_cast<int>(t * static_cast<float>(strip_w - 1));
            neutrino::draw_line(neutrino::point{marker_x, y - 4}, neutrino::point{marker_x, y + row_height + 3},
                                neutrino::colors::white);
            swatch(left + strip_w + 20, y, right - (left + strip_w + 20), row_height, neutrino::lerp(from(), to(), t));
        }

        // 2. Straight alpha needs the blend mode to show transparency; a premultiplied colour
        //    carries the same visible result in its rgb, here drawn against black.
        void draw_premultiply_row(int y) const {
            constexpr int steps = 16;
            constexpr int cell = (right - left) / steps;
            constexpr int half = row_height / 2;

            for (int x = left; x < right; x += 8) {
                for (int yy = y; yy < y + half; yy += 8) {
                    const bool dark = ((x - left) / 8 + (yy - y) / 8) % 2 == 0;
                    swatch(x, yy, 8, std::min(8, y + half - yy), dark ? neutrino::color{60, 60, 70} : neutrino::color{110, 110, 120});
                }
            }

            neutrino::set_draw_blend_mode(neutrino::blend_mode::blend);
            for (int i = 0; i < steps; ++i) {
                const auto alpha = static_cast <std::uint8_t>(i * 255 / (steps - 1));
                neutrino::color c = from();
                c.a = alpha;
                swatch(left + i * cell, y, cell, half, c);
            }
            neutrino::set_draw_blend_mode(neutrino::blend_mode::none);

            for (int i = 0; i < steps; ++i) {
                const auto alpha = static_cast <std::uint8_t>(i * 255 / (steps - 1));
                neutrino::color c = from();
                c.a = alpha;
                neutrino::color pre = c.premultiply(); // rgb scaled by alpha
                pre.a = 255;                           // drawn opaque: what straight alpha over black shows
                swatch(left + i * cell, y + half, cell, half, pre);
            }
        }

        // 3. luminance() weighs green most and blue least (BT.709); to_grayscale() repeats it
        //    into all three channels.
        static void draw_grayscale_row(int y) {
            constexpr int cell = (right - left) / static_cast<int>(palette.size());
            constexpr int half = row_height / 2;
            for (std::size_t i = 0; i < palette.size(); ++i) {
                const int x = left + static_cast<int>(i) * cell;
                swatch(x, y, cell - 2, half, palette[i]);
                swatch(x, y + half, cell - 2, half, palette[i].to_grayscale());

                // A tick whose length is the luminance, 0..255 mapped onto the cell width.
                const int tick = palette[i].luminance() * (cell - 2) / 255;
                neutrino::draw_line(neutrino::point{x, y + row_height + 2}, neutrino::point{x + tick, y + row_height + 2},
                                    neutrino::colors::white);
            }
        }

        // 4. + adds every channel (alpha too) and saturates at 255: light mixing.
        void draw_additive_row(int y) const {
            using neutrino::colors::red;
            using neutrino::colors::blue;
            const neutrino::color green{0, 255, 0};

            constexpr int seg = 80;
            const std::array segments{red, red + green, red + green + blue, green + blue, blue};
            for (std::size_t i = 0; i < segments.size(); ++i) {
                swatch(left + static_cast<int>(i) * seg, y, seg, row_height, segments[i]);
            }

            // A dim colour accumulating: c, c+c, c+c+c, ... until channels clip at 255.
            const neutrino::color dim = from() * 0.25f;
            neutrino::color acc = dim;
            const int start = left + 5 * seg + 20;
            constexpr int cell = 25;
            for (int x = start; x + cell <= right; x += cell) {
                swatch(x, y, cell - 2, row_height, acc);
                acc = acc + dim;
            }
        }

        // 5. * scales rgb (clamped) and leaves alpha alone: 0.0 is black, 1.0 the colour, 2.0 brighter.
        void draw_brightness_row(int y) const {
            constexpr int steps = 21;
            constexpr int cell = (right - left) / steps;
            for (int i = 0; i < steps; ++i) {
                const float factor = static_cast<float>(i) * 0.1f;
                swatch(left + i * cell, y, cell, row_height, from() * factor);
            }
            const int one_x = left + 10 * cell; // factor 1.0
            neutrino::draw_rect(neutrino::rect{one_x, y, cell, row_height}, neutrino::colors::white);
        }

        // 6. Compute in fcolor when the maths wants 0..1 floats, then convert for drawing.
        //    The conversion is explicit both ways and truncates: 0.5f becomes 127.
        void draw_float_row(int y) const {
            const neutrino::fcolor a{from()};
            const neutrino::fcolor b{to()};
            const float s = 0.5f + 0.5f * std::sin(m_time * 3.0f);
            const neutrino::fcolor pulse{
                a.r + (b.r - a.r) * s,
                std::sqrt(a.g * b.g),          // a float-only operation: geometric mean of green
                a.b * (1.0f - s) + b.b * s,
                1.0f,
            };
            swatch(left, y, 240, row_height, neutrino::color{pulse});

            // Truncation: fcolor gray 0.5 converts to 127; the X11 colors::gray is 190.
            swatch(left + 280, y, 60, row_height, neutrino::color{neutrino::colors::f::gray});
            swatch(left + 345, y, 60, row_height, neutrino::colors::gray);

            // Round trip: color -> fcolor -> color is exact for 8-bit values.
            const neutrino::color round_trip{neutrino::fcolor{from()}};
            swatch(left + 440, y, 70, row_height, from());
            swatch(left + 515, y, 70, row_height, round_trip);
        }

        std::size_t m_from{0};
        std::size_t m_to{7};
        float m_time{0.0f};
        bool m_paused{false};
    };

    class tutorial_app final : public neutrino::application {
    public:
        tutorial_app()
            : neutrino::application(make_config()) {
        }

    protected:
        std::unique_ptr<neutrino::base_scene> create_initial_scene() override {
            return std::make_unique<colors_scene>();
        }

    private:
        static neutrino::application_config make_config() {
            neutrino::application_config cfg;
            cfg.title = "Neutrino Drawing Tutorial 05: Colors";
            cfg.width = window_width;
            cfg.height = window_height;
            cfg.flags = neutrino::window_flags::resizable;
            cfg.logical_size = neutrino::dim{logical_width, logical_height};
            cfg.scale = neutrino::scale_mode::letterbox;
            return cfg;
        }
    };

} // namespace

NEUTRINO_MAIN(tutorial_app)
