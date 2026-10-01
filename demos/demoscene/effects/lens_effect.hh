#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <vector>
#include <euler/angles/radian.hh>

namespace demoscene {

/**
 * @brief Reconstruction of Bas van Gaalen's Spherical Refraction Lens (LENS.PAS).
 *
 * Demonstrates:
 * - 2.5D optical refraction simulation via hemispherical displacement fields.
 * - Displacement calculation: z = √(R² - x² - y²); offset = (R - z) / 2.
 * - Interactive lens positioning and automated harmonic bouncing.
 */
class lens_effect final : public effect_base {
public:
    lens_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Spherical Refraction Lens";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "LENS.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "z = √(R² - x² - y²); Δp = (R - z)/2; P_screen(p) = P_bg(p + Δp)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Hemispherical optical distortion map refracting and magnifying the background scene in real time.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Arrows: Steer Lens | Space: Toggle Auto-Bounce | R: Reset";
    }

private:
    void precompute_lens();
    void build_background();

    static constexpr int radius = 32;
    static constexpr int diameter = radius * 2;

    vga_canvas m_canvas;
    std::vector<int8_t> m_displacement_map; // (diameter x diameter)
    std::vector<uint8_t> m_background;      // (320 x 200)

    float m_lens_x = 160.0f;
    float m_lens_y = 100.0f;
    euler::radian<float> m_bounce_phase_x{0.0f};
    euler::radian<float> m_bounce_phase_y{1.5f};
    bool m_auto_bounce = true;
};

} // namespace demoscene
