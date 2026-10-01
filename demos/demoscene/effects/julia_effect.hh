#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <euler/complex/complex.hh>
#include <string_view>
#include <vector>

namespace demoscene {

/**
 * @brief Reconstruction of Bas van Gaalen's Julia Fractal (FRACTAL3.PAS / FRACZOOM.PAS).
 *
 * Demonstrates:
 * - Real-time complex dynamics z_{n+1} = z_n^2 + c using euler::complex<float>.
 * - Interactive complex plane panning and smooth exponential zooming.
 * - Dynamic parameter morphing c(t) generating hypnotic continuous geometric transformations.
 * - In-place VGA DAC palette cycling for zero-overhead animation.
 */
class julia_effect final : public effect_base {
public:
    julia_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Realtime Julia Fractal";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "FRACTAL3.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "z_{n+1} = z_n^2 + c; z, c ∈ ℂ; euler::complex<float>; |z|² ≤ 4";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Real-time complex plane Julia set with dynamic orbit morphing, exponential zoom, and palette cycling.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Arrows: Pan | W/S: Zoom | Space: Morph Orbit | 1..4: Classic Seeds | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();

    vga_canvas m_canvas;

    float m_center_x = 0.0f;
    float m_center_y = 0.0f;
    float m_zoom = 1.0f;

    // Complex parameter c = cr + i * ci
    euler::complex<float> m_c{-0.7f, 0.27015f};

    bool m_animating_c = true;
    float m_orbit_time = 0.0f;
    float m_palette_timer = 0.0f;

    static constexpr int max_iterations = 48;
};

} // namespace demoscene
