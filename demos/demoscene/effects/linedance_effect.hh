#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <vector>
#include <euler/vector/vector.hh>

namespace demoscene {

/**
 * @brief Dynamic Vector Line Dancing, Harmonic Spring Chains & Kaleidoscopic Ribbons.
 *
 * Origin: Classic demoscene vector effect (Johan Gardhage / linedance3.cpp).
 *
 * Demonstrates:
 * - 180-node harmonic damped spring chain: P_i = (P_i + P_{i-1}) / (2 + k/N).
 * - Multi-frequency Lissajous orbital driving attractor.
 * - 4-way and 8-way kaleidoscopic planar mirror symmetry.
 * - CRT phosphor persistence decay and luminous line rasterization.
 */
class linedance_effect final : public effect_base {
public:
    linedance_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Kaleidoscope Line Dance";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "linedance3.cpp (Johan Gardhage, 1990s Demoresearch)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Johan Gardhage";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "P_i = (P_i + P_{i-1}) / (2 + k/N);  Sym: (x, y), (W-x, y), (x, H-y), (W-x, H-y)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Coupled harmonic spring chain driven by Lissajous orbits with 4-way kaleidoscopic mirror symmetry.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Symmetry Mode | +/-: Damping/Elasticity | A: Motion Blur | C: Theme | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();

    vga_canvas m_canvas;

    static constexpr size_t chain_nodes = 180;
    std::vector<euler::vec2<float>> m_points;

    int m_mode = 0;          // 0 = 4-Way Kaleidoscope, 1 = 8-Way Mandala, 2 = Dual Ribbon DNA
    int m_theme = 0;         // 0 = Electric Cyan, 1 = Sunset Flame, 2 = Aurora Emerald, 3 = Amiga Copper
    bool m_motion_blur = true;

    float m_damping = 0.85f;
    float m_time = 0.0f;
};

} // namespace demoscene
