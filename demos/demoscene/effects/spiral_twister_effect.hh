#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <vector>
#include <euler/angles/radian.hh>
#include <euler/complex/complex.hh>

namespace demoscene {

/**
 * @brief Reconstruction and expansion of Bas van Gaalen's Spiral effects (SPIRAL.PAS / SCR_SPRL.PAS).
 *
 * Demonstrates:
 * - 3D Gouraud-shaded twisting column (classic demoscene twister) with harmonic wobbles.
 * - Logarithmic / Archimedean polar spiral vortex evaluated via euler::complex<float>.
 * - Amiga-style sine spiral ribbon scroller with VGA font bitpatterns.
 * - Real-time palette cycling and shading.
 */
class spiral_twister_effect final : public effect_base {
public:
    spiral_twister_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Spiral & Twister";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "SPIRAL.PAS & SCR_SPRL.PAS (1994-1995)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "θ(y,t) = ω·t + y·k_t + A·sin(k_w·y); r = |z|, θ = arg(z)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Classic demoscene 3D twisting column, logarithmic polar spiral vortex, and Amiga sine ribbon.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Mode (3D Twister, Polar Spiral, Sine Ribbon) | C: Palette | Up/Down: Twist | Left/Right: Speed | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();
    void render_twister_column();
    void render_polar_spiral();
    void render_sine_ribbon();

    vga_canvas m_canvas;

    float m_time = 0.0f;
    float m_rot_speed = 2.4f;
    float m_twist_rate = 0.035f;

    int m_mode = 0;         // 0 = 3D Twister Column, 1 = Polar Spiral Vortex, 2 = Amiga Sine Ribbon
    int m_palette_theme = 0; // 0 = Amiga Copper Rainbow, 1 = Cyberpunk Neon, 2 = Solar Flame
};

} // namespace demoscene
