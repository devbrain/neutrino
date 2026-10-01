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
 * @brief Reconstruction of Bas van Gaalen's 3D Dot Matrix Fields (DOTS1.PAS / DOTS2.PAS).
 *
 * Demonstrates:
 * - 800+ particle mathematical vector field evaluated with multi-harmonic sine waves.
 * - Multi-axis quaternion 3D rotation via euler::quaternion<float>.
 * - Depth-cued 1/z perspective scaling and color gradient mapping.
 * - Simulated phosphor persistence and ghost particle trails.
 */
class dots_wave_effect final : public effect_base {
public:
    dots_wave_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Sine Wave Dot Matrix";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "DOTS1.PAS & DOTS2.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "P_i(t) = q · (x(i,t), y(i,t), z(i,t)) · q⁻¹; x,y,z = Σ A_k · sin(ω_k i ± t)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Classic 3D undulating vector dot matrix with multi-harmonic waves and phosphor trails.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Arrows: Rotate | W/S: Zoom | Space: Pattern (Ribbon, Torus, Cube Matrix) | C: Theme | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();
    [[nodiscard]] euler::vec3<float> compute_dot_position(int index, float t) const;

    vga_canvas m_canvas;

    static constexpr int num_dots = 850;

    euler::quaternion<float> m_orientation;
    euler::vec3<float> m_angular_velocity{0.4f, 0.7f, 0.2f};
    float m_zoom = 220.0f;
    float m_time = 0.0f;

    int m_pattern_mode = 0;   // 0 = Undulating Ribbon Wave, 1 = Rotating Torus Ring, 2 = 3D Cube Matrix Grid
    int m_color_theme = 0;    // 0 = Cyan/Ice Blue, 1 = Synthwave Magenta/Purple, 2 = Emerald Green
};

} // namespace demoscene
