#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <vector>
#include <random>

namespace demoscene {

/**
 * @brief Asynchronous Twinkling & Blinking Stars with Diffraction Flare Bitmasks.
 *
 * Origin: STARS2.PAS by Bas van Gaalen (1995).
 *
 * Demonstrates:
 * - Asynchronous stellar twinkling with 20-phase luminance envelopes.
 * - Dual 5x5 diffraction spike flare bitmasks (compact cross -> extended stellar flare).
 * - Multi-band DAC palette organization (6 spectral color groups: Red, Green, Blue, Yellow, Cyan, White).
 * - Constellation line graph synthesis between neighboring stellar peaks.
 * - Real-time meteor / shooting star streaks.
 */
class blinking_stars_effect final : public effect_base {
public:
    blinking_stars_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Blinking / Twinkling Stars";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "STARS2.PAS (Bas van Gaalen, 1995)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "L(t) = f(phase); bitmask[phase > 10, x, y]; P = bitmask + col + phase";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Asynchronous twinkling stars with 20-phase luminance envelopes, dual 5x5 flare bitmasks, and constellation graphs.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Sky Mode | +/-: Star Count | S: Shooting Star | C: Theme | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();
    void reset_stars();
    void spawn_meteor();

    vga_canvas m_canvas;

    struct star {
        int x;
        int y;
        int phase;
        int color_group;
        int duration;
        bool active;
    };

    struct meteor {
        float x;
        float y;
        float vx;
        float vy;
        float life;
        bool active;
    };

    static constexpr size_t max_stars = 300;
    std::vector<star> m_stars;
    size_t m_star_count = 120;

    std::vector<meteor> m_meteors;

    int m_sky_mode = 0;      // 0 = Authentic 1995 Stars, 1 = Constellation Graph, 2 = Galactic Cluster
    int m_theme = 0;         // 0 = Authentic Spectral Rainbow, 1 = Diamond White, 2 = Cyberpunk Neon
    float m_meteor_timer = 0.0f;
    float m_nebula_time = 0.0f;

    std::mt19937 m_rng{1995};
};

} // namespace demoscene
