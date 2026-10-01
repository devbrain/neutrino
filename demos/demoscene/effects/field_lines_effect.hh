#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <vector>
#include <euler/vector/vector.hh>
#include <euler/vector/vector_ops.hh>

namespace demoscene {

/**
 * @brief High-performance reconstruction of Bas van Gaalen's Electric Field Lines (FIELD.PAS).
 *
 * Demonstrates:
 * - Coulomb's Law vector field integration: E(r) = Σ qi · (r - ri) / |r - ri|³.
 * - Field line numerical integration using Euler/Runge-Kutta steps.
 * - Real-time SIMD-accelerated equipotential scalar potential field coloring.
 * - Interactive positive and negative charge placement and physics simulation.
 * - Authentic 1994 FIELD.PAS coordinate grid mode.
 */
class field_lines_effect final : public effect_base {
public:
    field_lines_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Electric Field Lines";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "FIELD.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "E(r) = Σ q_i · (r - r_i) / |r - r_i|³; r_{k+1} = r_k + Δs · E / |E|";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "High-performance electrostatic field line simulation with SIMD equipotential contours and retro grid.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Randomize | M: Mode (Heatmap / Retro Grid / Lines Only) | C: Theme | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    struct charge {
        euler::vec2<float> pos;
        float q; // Positive (+1) or Negative (-1)
        float radius = 6.0f;
    };

    void init_palette();
    void reset_charges();
    void compute_field_fast(float px, float py, float& ex, float& ey) const noexcept;
    void trace_line(float start_x, float start_y, float sign);

    void render_heatmap();
    void render_retro_grid();
    void render_field_lines();
    void render_charges();

    vga_canvas m_canvas;

    std::vector<charge> m_charges;
    std::array<float, 1024> m_sin_lut{};

    float m_time = 0.0f;
    int m_display_mode = 0; // 0 = Heatmap + Lines, 1 = Retro Grid + Lines (FIELD.PAS), 2 = Lines Only
    int m_color_theme = 0;
};

} // namespace demoscene
