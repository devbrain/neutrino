#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <vector>
#include <euler/angles/radian.hh>
#include <euler/quaternion/quaternion.hh>
#include <euler/quaternion/quaternion_ops.hh>
#include <euler/vector/vector.hh>
#include <euler/vector/vector_ops.hh>

namespace demoscene {

/**
 * @brief Reconstruction of Bas van Gaalen's Lissajous Phosphor Trails (FADEPLOT.PAS).
 *
 * Demonstrates:
 * - High-harmonic 3D Lissajous knot trajectories and parametric space curves.
 * - Multi-axis orientation control using euler::quaternion<float>.
 * - Simulated CRT phosphor persistence and decay via in-place framebuffer fading.
 * - Multiple iconic retro phosphor color schemes (P1 Green, P3 Amber, P4 White, Cyan).
 */
class fadeplot_effect final : public effect_base {
public:
    fadeplot_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Phosphor Lissajous Trails";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "FADEPLOT.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "x(t)=Ax·sin(ωx·t+φ); y(t)=Ay·sin(ωy·t); z(t)=Az·cos(ωz·t); P(t) ← P(t) - decay";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "3D Lissajous curves and topological knots with decaying CRT phosphor persistence trails.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Arrows: Rotate | W/S: Zoom | Space: Knot Curve | C: Phosphor CRT | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();
    [[nodiscard]] euler::vec3<float> evaluate_curve(float t) const;

    vga_canvas m_canvas;

    euler::quaternion<float> m_orientation;
    euler::vec3<float> m_angular_velocity{0.4f, 0.7f, 0.2f};
    float m_zoom = 220.0f;
    float m_time = 0.0f;

    int m_curve_mode = 0;    // 0 = Trefoil Knot, 1 = 3:4:5 Lissajous Ribbon, 2 = Torus Knot, 3 = Figure-8
    int m_phosphor_theme = 0;// 0 = P1 Classic Green CRT, 1 = P3 Amber CRT, 2 = Cyberpunk Cyan/Violet
    int m_decay_rate = 3;    // Decay intensity per frame
};

} // namespace demoscene
