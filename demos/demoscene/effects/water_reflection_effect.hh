#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <vector>

namespace demoscene {

/**
 * @brief Water Surface Reflection, Horizon Ripling & Optical Wave Distortion.
 *
 * Origin: Classic demoscene scenery effect (Pierre-Jean Turpeau / reflet.c, Amiga/PC intro art).
 *
 * Demonstrates:
 * - Dynamic water mirror reflection with compound trigonometric wave displacement:
 *   y_refl(j) = H_sky - j + A_v · sin(j/λ_v + ω_v t) · (j / d);
 *   x_refl(j) = A_h · cos(j/λ_h + ω_h t) · (j / d).
 * - Multi-bank VGA palette partitioning (0..127 skyline / 128..255 tinted watery reflection).
 * - Procedural demoscene procedural skylines (Cyberpunk Metropolis, Alpine Sunset, Starry Citadel).
 * - Interactive wind agitation, wave amplitude, and daylight cycle.
 */
class water_reflection_effect final : public effect_base {
public:
    water_reflection_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Water Surface Reflection";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "reflet.c (Pierre-Jean Turpeau, 2020)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Pierre-Jean Turpeau";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "y' = H_sky - j + A·sin(j/3 + ω t)·j/20;  x' = A·cos(j/4 + ω t)·j/25";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "City skyline & horizon reflected across an undulating perspective water plane with wave refraction.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Skyline Theme | Up/Down: Wave Amplitude | Left/Right: Wind Speed | C: Palette | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();
    void generate_skyline();

    vga_canvas m_canvas;

    static constexpr int horizon_y = 100;
    std::vector<uint8_t> m_skyline_buffer;

    int m_skyline_type = 0;  // 0 = Cyberpunk Metropolis, 1 = Alpine Sunset Peaks, 2 = Cosmic Citadel
    int m_theme = 0;         // 0 = Twilight Indigo, 1 = Golden Amber Sunset, 2 = Emerald Aurora
    float m_wave_amp = 1.0f;
    float m_wind_speed = 1.0f;
    float m_time = 0.0f;
};

} // namespace demoscene
