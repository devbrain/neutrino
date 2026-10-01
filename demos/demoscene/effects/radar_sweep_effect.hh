#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <vector>
#include <euler/vector/vector.hh>
#include <euler/vector/vector_ops.hh>

namespace demoscene {

/**
 * @brief Reconstruction and expansion of Bas van Gaalen's Screen Sweep (SWEEP.PAS).
 *
 * Demonstrates:
 * - 360-degree rotating polar beam with Bresenham line sweep.
 * - Analog CRT phosphor persistence and exponential decay trails.
 * - Dynamic radar targets (blips) illuminated during beam passage with heading history.
 * - Concentric range rings, cardinal azimuth markers, and sonar/radar display modes.
 */
class radar_sweep_effect final : public effect_base {
public:
    radar_sweep_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Radar Scope Sweep";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "SWEEP.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "x = x_0 + R·cos(ω·t), y = y_0 + R·sin(ω·t); I_{k+1} = max(0, I_k - δ)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Rotating radar beam sweep with CRT phosphor persistence decay, range rings, and dynamic target blips.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Add Blips | C: Phosphor Theme | Up/Down: Sweep Speed | M: Grid Toggle | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    struct target_blip {
        euler::vec2<float> pos;
        euler::vec2<float> vel;
        float intensity = 0.0f;
    };

    void init_palette();
    void reset_targets();
    void draw_line(int x0, int y0, int x1, int y1, uint8_t color);

    vga_canvas m_canvas;

    std::vector<target_blip> m_targets;

    float m_angle = 0.0f;
    float m_sweep_speed = 2.0f; // Radians per second
    float m_time = 0.0f;

    bool m_show_grid = true;
    int m_color_theme = 0; // 0 = P31 Green, 1 = P20 Amber, 2 = Sonar Blue
};

} // namespace demoscene
