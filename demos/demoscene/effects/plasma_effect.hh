#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <euler/angles/radian.hh>

namespace demoscene {

/**
 * @brief Reconstruction of Bas van Gaalen's multi-wave Plasma (PLASMA1.PAS).
 *
 * Demonstrates:
 * - 2D wave superposition combining Cartesian and circular trigonometric fields.
 * - Genuine 1990s hardware palette cycling: rotating 256 colors without recomputing pixels.
 * - euler::radian phase animation.
 */
class plasma_effect final : public effect_base {
public:
    plasma_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Multi-Wave Sine Plasma";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "PLASMA1.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "C(x, y) = sin(k₁x + φ₁) + sin(k₂y + φ₂) + sin(k₃·dist(x,y) + φ₃); Palette Cycle";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Interference pattern of harmonic sine waves combined with continuous palette DAC rotation.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "P: Toggle Palette Cycling | Arrows: Cycle Speed | R: Reset";
    }

private:
    void rebuild_palette();

    vga_canvas m_canvas;

    euler::radian<float> m_phase_x{0.0f};
    euler::radian<float> m_phase_y{0.0f};
    euler::radian<float> m_phase_rad{0.0f};

    float m_cycle_timer = 0.0f;
    int m_cycle_speed = 2;
    bool m_cycling_enabled = true;
};

} // namespace demoscene
