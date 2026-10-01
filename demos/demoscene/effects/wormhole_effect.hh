#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <euler/angles/radian.hh>
#include <euler/vector/vector.hh>

namespace demoscene {

/**
 * @brief Reconstruction of Bas van Gaalen's iconic Wormhole effect (WORMHOLE.PAS).
 *
 * Demonstrates:
 * - Concentric polar circle extrusion along harmonic Lissajous orbits.
 * - Modern euler::radian<float> angle arithmetic replacing integer trig tables.
 * - Depth-cued palette gradients on a Mode 13h canvas.
 */
class wormhole_effect final : public effect_base {
public:
    wormhole_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Second Reality Wormhole";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "WORMHOLE.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen (homage to Future Crew)";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "Polar rings: P(r, θ) = Center(t, r) + (r·cos θ, r·sin θ)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Concentric rings swept along a dual-frequency Lissajous curve creating a tunnel illusion.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Arrow Keys: Adjust wormhole sway & speed | R: Reset";
    }

private:
    vga_canvas m_canvas;
    euler::radian<float> m_phase_x{0.0f};
    euler::radian<float> m_phase_y{0.0f};
    float m_speed = 1.0f;
    float m_sway_amp_x = 60.0f;
    float m_sway_amp_y = 45.0f;
};

} // namespace demoscene
