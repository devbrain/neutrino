#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <string>
#include <vector>

namespace demoscene {

/**
 * @brief Reconstruction of the classic DOS/Amiga DYPP Sinus Scroller (SCR_DYPP.PAS).
 *
 * Demonstrates:
 * - Different-Y-Pixel-Position (DYPP) column displacement along compound sine waves.
 * - Dynamic 8x8 BIOS font glyph rasterization and horizontal smooth pixel scrolling.
 * - 3D shaded ribbon extrusion with drop shadow and multi-gradient color ramps.
 * - Interactive wave harmonics, amplitude modulation, and text message switching.
 */
class dypp_effect final : public effect_base {
public:
    dypp_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "DYPP Sinus Scroller";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "SCR_DYPP.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "y(x, t) = y_0 + A_1 · sin(k_1 x + ω_1 t) + A_2 · cos(k_2 x + ω_2 t)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Classic 90s sine wave text scroller with per-column vertical displacement and ribbon extrusion.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Up/Down: Amplitude | Left/Right: Scroll Speed | Space: Change Text | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();

    vga_canvas m_canvas;

    float m_scroll_pos = 0.0f;
    float m_scroll_speed = 90.0f;
    float m_time = 0.0f;
    float m_amp1 = 28.0f;
    float m_amp2 = 14.0f;

    size_t m_current_text_idx = 0;
    std::vector<std::string> m_messages;

    struct star {
        float x, y, speed;
        uint8_t color;
    };
    std::vector<star> m_stars;
};

} // namespace demoscene
