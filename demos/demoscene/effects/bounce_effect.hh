#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <vector>

namespace demoscene {

/**
 * @brief Reconstruction of Bas van Gaalen's Bouncing Spheres (BOUNCE.PAS).
 *
 * Demonstrates:
 * - Kinematic bouncing physics with gravity integration, velocity damping, and restitution.
 * - Dynamic squash & stretch deformation upon floor impact preserving sphere volume.
 * - Real-time perspective elliptical shadow projection scaling with altitude.
 * - 3D spherical Lambertian diffuse shading with specular highlight.
 */
class bounce_effect final : public effect_base {
public:
    bounce_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Bouncing Physics Spheres";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "BOUNCE.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "y(t) = y₀ + v·t - ½g·t²; v' = -e·v; Squash: rx·ry = R²";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Realistic bouncing spheres with restitution physics, squash & stretch, dynamic shadows, and 3D specular shine.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Drop Ball | Up/Down: Gravity | Left/Right: Restitution | C: Color Theme | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    struct ball {
        float x = 0.0f;
        float y = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        float radius = 16.0f;
        float squash = 0.0f;
        uint8_t base_color_idx = 40;
    };

    void init_palette();
    void reset_balls();
    void render_shaded_sphere(int cx, int cy, float rx, float ry, uint8_t base_col);

    vga_canvas m_canvas;

    std::vector<ball> m_balls;

    float m_gravity = 480.0f;
    float m_restitution = 0.82f;
    float m_floor_y = 175.0f;
    int m_theme = 0; // 0 = Multi-Colored Rubber, 1 = Polished Chrome, 2 = Fiery Magma
};

} // namespace demoscene
