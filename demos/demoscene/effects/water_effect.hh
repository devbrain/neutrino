#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <vector>
#include <euler/vector/vector.hh>

namespace demoscene {

/**
 * @brief 2D Wave Equation Fluid Simulation with Optical Caustics and Refraction.
 *
 * Origin: Classic 1990s demo effect (Tran / Renaissance, popularized in retro-demoeffects).
 *
 * Demonstrates:
 * - Numerical integration of 2D discrete wave equation: h_{t+1} = (Σ h_neighbors / 2 - h_{t-1}) · damping.
 * - Real-time surface gradient normals ∇h and Snell's law optical refraction ray-bending.
 * - Procedural pool mosaic background with dynamic caustic light refraction.
 * - Interactive rain drops, viscosity controls, and fluid themes.
 */
class water_effect final : public effect_base {
public:
    water_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "2D Wave Equation Water";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "water.cpp / symwater (1990s Demoresearch)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Johan Gardhage & Tran";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "h_{t+1} = (½∇²h_t - h_{t-1})·d; (x', y') = (x, y) - r·∇h";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Real-time discrete wave equation fluid simulation with caustics, wave propagation, and refractive ray-bending.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Splash Drops | Arrows: Move Ripple | Up/Down: Viscosity | C: Pool Theme | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();
    void generate_background();
    void drop_splash(int x, int y, float depth, int radius);

    vga_canvas m_canvas;

    std::vector<float> m_height1;
    std::vector<float> m_height2;
    std::vector<uint8_t> m_background;

    float m_damping = 0.985f;
    float m_refraction = 16.0f;
    float m_ripple_x = 160.0f;
    float m_ripple_y = 100.0f;
    float m_time = 0.0f;
    int m_theme = 0; // 0 = Azure Pool, 1 = Emerald Lagoon, 2 = Molten Lava
};

} // namespace demoscene
