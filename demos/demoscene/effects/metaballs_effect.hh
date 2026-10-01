#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <vector>
#include <euler/vector/vector.hh>
#include <euler/vector/vector_ops.hh>

namespace demoscene {

/**
 * @brief 2D Scalar Field Isosurface Metaballs and Liquid Blobs.
 *
 * Origin: Classic demoscene effect (retro-demoeffects / oldskewlish).
 *
 * Demonstrates:
 * - Real-time potential summation: V(x, y) = Σ R_i² / (||p - p_i||² + ε).
 * - Organic fluid merging and splitting without explicit geometry.
 * - Continuous iridescent equipotential mapping, thresholding, and contour bands.
 * - Multi-harmonic Lissajous orbits and liquid metal chrome shading.
 */
class metaballs_effect final : public effect_base {
public:
    metaballs_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Isosurface Metaballs";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "metaballs.cpp & blobs.cpp (retro-demoeffects)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Johan Gardhage";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "V(x,y) = Σ R_i² / ((x-x_i)² + (y-y_i)² + ε); V ≥ V_iso";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Organic scalar potential field metaballs merging and dividing in real-time with iridescent shading.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Mode (Smooth, Solid Blob, Contours) | C: Theme | Up/Down: Size | W/S: Speed | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    struct metaball {
        euler::vec2<float> pos;
        euler::vec2<float> vel;
        float radius = 28.0f;
    };

    void init_palette();
    void reset_balls();

    vga_canvas m_canvas;

    std::vector<metaball> m_balls;

    float m_time = 0.0f;
    float m_speed = 1.0f;
    float m_threshold = 1.0f;
    int m_mode = 0;         // 0 = Continuous Iridescent, 1 = Solid Organic Blob, 2 = Equipotential Contours
    int m_color_theme = 0;  // 0 = Liquid Chrome, 1 = Synthwave Magenta, 2 = Emerald Acid
};

} // namespace demoscene
