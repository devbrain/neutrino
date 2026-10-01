#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <euler/angles/radian.hh>
#include <vector>

namespace demoscene {

/**
 * @brief Reconstruction of Bas van Gaalen's 2D Sine Texture Mapper (SINMAP.PAS).
 *
 * Demonstrates:
 * - Real-time 2D grid deformation using compound sinusoidal displacement fields.
 * - Dynamic texture mapping with forward/inverse mesh warping.
 * - Authentic 1994 striped Dutch flag / retro demoscene checkerboard patterns.
 * - Parameterized wave harmonics, amplitude modulation, and palette animation.
 */
class sinmap_effect final : public effect_base {
public:
    sinmap_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Sinusoidal Mesh Warp";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "SINMAP.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "x' = x + Ax · sin(ky·y + ωt); y' = y + Ay · cos(kx·x - ωt)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Real-time 2D procedural texture mesh deformation with compound sinusoidal ripples.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Up/Down: Amplitude | Left/Right: Frequency | Space: Pattern | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();
    void generate_texture_pattern();

    vga_canvas m_canvas;
    static constexpr int tex_w = 160;
    static constexpr int tex_h = 100;
    std::vector<uint8_t> m_texture;

    float m_time = 0.0f;
    float m_amp_x = 18.0f;
    float m_amp_y = 14.0f;
    float m_freq = 1.0f;
    int m_pattern_mode = 0; // 0 = Dutch Tri-color Flag, 1 = Demoscene Checkerboard, 2 = Concentric Vortex
};

} // namespace demoscene
