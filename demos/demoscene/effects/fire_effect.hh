#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <random>
#include <vector>

namespace demoscene {

/**
 * @brief Reconstruction of classic DOS/Demoscene Fire cellular automata (3D_FCUBE.PAS / Fast Fire).
 *
 * Demonstrates:
 * - Upward thermal convection simulation with 4-neighborhood dissipation averaging.
 * - Dynamic wind deflection and cooling decay parameterization.
 * - Classic 256-color heat palette (Black -> Red -> Yellow -> White).
 * - Interactive spark ignition and combustion emitters.
 */
class fire_effect final : public effect_base {
public:
    fire_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Convection Fire";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "3D_FCUBE.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen & Jare";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "heat(x, y) = max(0, avg(heat(x±wind, y+1..2)) - decay)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Real-time thermal convection fire simulation with interactive wind, cooling, and spark bursts.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Left/Right: Wind | Up/Down: Cooling | Space: Ignition Burst | 1..3: Emitter Mode";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void step_simulation();
    void seed_fuel();

    vga_canvas m_canvas;
    std::array<uint8_t, vga_canvas::pixel_count> m_fire_buffer{};

    int m_wind = 0;              // Wind offset (-2 to +2 pixels shift)
    int m_cooling = 2;           // Heat loss per convection step
    int m_emitter_mode = 0;      // 0 = Full floor inferno, 1 = Center pyre, 2 = Dancing torches
    float m_sim_timer = 0.0f;
    float m_time = 0.0f;
    bool m_burst_active = false;
    float m_burst_timer = 0.0f;

    std::mt19937 m_rng{1337};
};

} // namespace demoscene
