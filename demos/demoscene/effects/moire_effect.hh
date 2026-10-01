#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

namespace demoscene {

/**
 * @brief Dynamic Moiré Optical Interference Patterns & Radial Harmonic Fringes.
 *
 * Origin: Classic demoscene optical effect (Pierre-Jean Turpeau / moire.c, Future Crew Panic).
 *
 * Demonstrates:
 * - Analytical dual/triple concentric ring superpositions on moving Lissajous centers.
 * - Flat XOR distance bitmask interference: ((r1 ^ r2) >> 4) & 1.
 * - Continuous wave superposition: I(x, y) = sin(ω₁ r₁) + sin(ω₂ r₂) + sin(ω₃ r₃).
 * - Row-invariant dy² hoisting and high-performance Mode 13h inner loop evaluation.
 */
class moire_effect final : public effect_base {
public:
    moire_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Moiré Optical Interference";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "moire.c (Pierre-Jean Turpeau, 2019)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Pierre-Jean Turpeau";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "r_k = √((x-x_k)² + (y-y_k)²);  val = (r₁ ⊕ r₂) or Σ sin(ω_k r_k)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Analytical concentric wave centers generating animated hyperbolic and elliptical moiré fringes.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Interference Mode | Arrows: Move Center 1 | +/-: Density | C: Theme | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();

    vga_canvas m_canvas;

    float m_cx1 = 160.0f;
    float m_cy1 = 100.0f;
    float m_cx2 = 160.0f;
    float m_cy2 = 100.0f;
    float m_cx3 = 160.0f;
    float m_cy3 = 100.0f;

    int m_mode = 0;          // 0 = Flat XOR Rings, 1 = Continuous Wave Superposition, 2 = 3-Center Sierpinski Lattice
    int m_theme = 0;         // 0 = Monochrome Opal, 1 = Electric Rainbow, 2 = Synthwave Neon, 3 = Thermal Infrared
    float m_frequency = 1.0f;
    float m_time = 0.0f;
    bool m_manual_control = false;
};

} // namespace demoscene
