#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <vector>
#include <euler/vector/vector.hh>
#include <euler/quaternion/quaternion.hh>
#include <euler/quaternion/quaternion_ops.hh>

namespace demoscene {

/**
 * @brief 3D Strange Attractor Particle Systems & Chaotic Dynamics.
 *
 * Origin: Classic demoscene mathematical effect (Pierre-Jean Turpeau / oldskewlish, Pickover 1990).
 *
 * Demonstrates:
 * - Numerical integration of non-linear differential systems (Lorenz, Pickover 3D, Rössler, Aizawa).
 * - Full 3D quaternion rotation (euler::quaternion<float>) and depth-cued perspective projection.
 * - Mode 13h phosphor trail attenuation simulation (CRT persistence glow).
 * - Additive density accumulation splatting (white-hot energetic convergence).
 */
class attractors_effect final : public effect_base {
public:
    attractors_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "3D Strange Attractors";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "attractors.c (oldskewlish / Turpeau 2020)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Pierre-Jean Turpeau & E. Lorenz";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override;
    [[nodiscard]] std::string_view description() const noexcept override;
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Attractor Type | +/-: Density | Arrows: Rotate 3D | W/S: Zoom | A: Phosphor | C: Theme";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();
    void reset_simulation();
    void step_simulation(int steps);

    vga_canvas m_canvas;

    static constexpr size_t max_points = 8192;
    std::vector<euler::vec3<float>> m_points;
    euler::vec3<float> m_current_pos{0.1f, 0.0f, 0.0f};
    size_t m_write_head = 0;
    size_t m_valid_points = 0;
    size_t m_active_points = 4096;

    int m_attractor_type = 0; // 0=Lorenz, 1=Pickover, 2=Rossler, 3=Aizawa
    int m_palette_theme = 0;  // 0=Neon Cyan, 1=Phosphor Green, 2=Solar Amber, 3=Cyberpunk
    bool m_attenuate = true;

    euler::quaternion<float> m_orientation{euler::quaternion<float>::identity()};
    euler::vec3<float> m_angular_velocity{0.15f, 0.45f, 0.1f};
    float m_zoom = 180.0f;
    float m_sim_time = 0.0f;
};

} // namespace demoscene
