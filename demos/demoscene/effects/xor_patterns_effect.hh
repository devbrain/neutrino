#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

namespace demoscene {

/**
 * @brief Bitwise XOR Fractal Automata, Concentric Circles & Sierpinski Carpets.
 *
 * Origin: Classic 1980s-1990s demoscene staple (Johan Gardhage / xorcircles.cpp, Sean Palmer).
 *
 * Demonstrates:
 * - Bitwise coordinate superposition generating infinite self-similar Sierpinski fractal lattices:
 *   C(x, y) = (x ⊕ y) mod 256.
 * - Dynamic moving multi-center circular distance XOR: C(x, y) = r₁(x, y) ⊕ r₂(x, y).
 * - Animated 4-bit modular XOR automata mandala.
 * - In-place palette cycling producing continuous hypnotic motion.
 */
class xor_patterns_effect final : public effect_base {
public:
    xor_patterns_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "XOR Fractals & Circles";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "xorcircles.cpp (Johan Gardhage, 1990s Demoresearch)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Johan Gardhage & Sean Palmer";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "C(x, y) = r₁(x, y) ⊕ r₂(x, y);  Sierpinski: (x ⊕ y) mod 256";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Bitwise coordinate superposition and orbital concentric circle XOR generating infinite Sierpinski fractals.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Pattern Mode | +/-: Scale / Zoom | Arrows: Pan | C: Theme | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();

    vga_canvas m_canvas;

    int m_mode = 0;          // 0 = Orbital XOR Circles, 1 = Sierpinski Fractal Carpet, 2 = Bitwise Automata Mandala
    int m_theme = 0;         // 0 = Neon Amethyst, 1 = Amber Flame, 2 = Laser Cyan, 3 = Phosphor Green
    float m_scale = 1.0f;
    float m_pan_x = 0.0f;
    float m_pan_y = 0.0f;
    float m_time = 0.0f;
};

} // namespace demoscene
