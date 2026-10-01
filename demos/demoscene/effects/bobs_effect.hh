#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <euler/angles/radian.hh>
#include <euler/vector/vector.hh>

namespace demoscene {

/**
 * @brief Reconstruction of Bas van Gaalen's Shaded Bobs (SHADEBOB.PAS).
 *
 * Demonstrates:
 * - Pixel accumulation buffer: buffer[p] = (buffer[p] + stamp[u, v]) mod 64.
 * - Multi-harmonic Lissajous orbits creating interwoven ribbon filaments.
 * - Modern euler::radian angle accumulation.
 * - Interactive trail decay and bob count tuning.
 */
class bobs_effect final : public effect_base {
public:
    bobs_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Translucent Shaded Bobs";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "SHADEBOB.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "P(x, y) ← (P(x, y) + Stamp(u, v)) mod 64; Lissajous: (A·cos(ω₁t), B·sin(ω₂t))";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Additive translucent pixel accumulation weaving self-intersecting glowing ribbons.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "C: Clear Canvas | D: Toggle Decay | Arrows: Adjust Speed | R: Reset";
    }

private:
    void stamp_bob(int center_x, int center_y);

    vga_canvas m_canvas;

    euler::radian<float> m_phase_a{0.0f};
    euler::radian<float> m_phase_b{1.2f};
    euler::radian<float> m_phase_c{2.4f};
    euler::radian<float> m_phase_d{3.6f};

    float m_speed = 1.0f;
    bool m_decay_enabled = false;
};

} // namespace demoscene
