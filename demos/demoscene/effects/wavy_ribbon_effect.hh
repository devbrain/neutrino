#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <vector>

namespace demoscene {

/**
 * @brief Compound Sine Wave Interference & Undulating 3D Silk Ribbon.
 *
 * Origin: WAVY.PAS by Bas van Gaalen (Feb 14, 1995).
 *
 * Demonstrates:
 * - 3-frequency trigonometric Fourier synthesis: y(x) = Σ A_k · sin(ω_k x + φ_k).
 * - Multi-phase accumulator stepping (add1=+1, add2=-1, add3=-1).
 * - Multi-strand 3D silk ribbon rendering with vertical scanline gradient rasterization.
 * - Real-time Fourier harmonic decomposition (displaying individual wave components).
 * - Authentic 1994 pastel seafoam/copper palette and CRT phosphor trails.
 */
class wavy_ribbon_effect final : public effect_base {
public:
    wavy_ribbon_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Compound Sine Wavy Ribbon";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "WAVY.PAS (Bas van Gaalen, 1995)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "y(x) = 50·sin(x+φ₁) + 25·cos(2x+φ₂) + 25·sin(2x+φ₃) + 99";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Multi-harmonic sine wave interference synthesis, 3D undulating silk ribbons, and Fourier decomposition.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Ribbon Mode | Up/Down: Amplitude | Left/Right: Speed | C: Palette | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_tables();
    void init_palette();

    vga_canvas m_canvas;

    std::array<int, 256> m_stab1{};
    std::array<int, 256> m_stab2{};
    std::array<int, 256> m_stab3{};

    std::array<uint8_t, 320> m_ctab{};
    std::array<uint8_t, 320> m_ptab{};

    int m_i1 = 0;
    int m_i2 = 25;
    int m_i3 = 100;

    int m_mode = 1;          // 0=Authentic 1994, 1=3D Silk Ribbon, 2=Phosphor Wave, 3=Fourier Spectrum
    int m_theme = 0;         // 0=Seafoam/Copper, 1=Cyberpunk Neon, 2=Aurora Borealis, 3=Golden Sunset
    float m_amplitude = 1.0f;
    float m_speed = 1.0f;
    float m_time = 0.0f;
};

} // namespace demoscene
