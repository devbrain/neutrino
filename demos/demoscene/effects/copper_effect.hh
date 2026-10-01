#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <vector>

namespace demoscene {

/**
 * @brief Reconstruction of the classic Amiga/DOS Copper & Raster Bars (COPPER5.PAS).
 *
 * Demonstrates:
 * - Real-time additive RGB raster bar wave interference.
 * - Cylindrical specular lighting gradients across metallic bars.
 * - Dynamic perspective mirror reflection floor.
 * - Multiple iconic color themes (Metallic Copper, Synthwave Neon, Rainbow Spectrum).
 */
class copper_effect final : public effect_base {
public:
    copper_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Rainbow Copper Bars";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "COPPER5.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "y_i(t) = mid_y + A_i · sin(ω_i t + φ_i); RGB(y) = Σ bar_i(y)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Floating metallic copper bars with additive sinusoidal interference and perspective reflection.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Up/Down: Bar count | Left/Right: Speed | Space: Palette | M: Floor Mirror";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    struct bar_desc {
        float freq;
        float amp;
        float phase;
        int height;
        sdlpp::color base_color;
    };

    void init_palette();

    vga_canvas m_canvas;
    std::vector<bar_desc> m_bars;

    float m_time = 0.0f;
    float m_speed = 1.0f;
    int m_bar_count = 6;
    int m_palette_theme = 0; // 0 = Metallic Copper, 1 = Synthwave Neon, 2 = Rainbow Spectrum
    bool m_mirror_floor = true;
};

} // namespace demoscene
