#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <vector>

namespace demoscene {

/**
 * @brief 3D Concentric Ring Dot Tunnel Fly-Through.
 *
 * Origin: dottunnel.cpp & dottunnel2.cpp by Johan Gardhage (Demoresearch).
 *
 * Demonstrates:
 * - 48 concentric circular rings of glowing dots receding into deep perspective.
 * - Continuous forward flight camera with seamless ring recycling.
 * - Compound harmonic Lissajous camera sway and spiral twist.
 * - Depth-cued perspective particle splatting and dynamic color banking.
 */
class dot_tunnel_effect final : public effect_base {
public:
    dot_tunnel_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "3D Concentric Ring Dot Tunnel";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "dottunnel.cpp (Johan Gardhage)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Johan Gardhage";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "x_s = x_c + (R·cos θ + x_sway) / z_i;  y_s = y_c + (R·sin θ + y_sway) / z_i";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Endless perspective flight through 48 glowing concentric particle rings with Lissajous tunnel sway.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Color Theme | T: Spiral Twist | Arrows: Sway Sway | +/-: Flight Speed | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    static constexpr int num_rings = 48;
    static constexpr int dots_per_ring = 64;

    void init_palette();
    void init_ring_coords();

    vga_canvas m_canvas;

    std::array<float, dots_per_ring> m_cos_table{};
    std::array<float, dots_per_ring> m_sin_table{};

    int m_theme = 0; // 0=Cyber Neon, 1=Emerald Matrix, 2=Solar Amber, 3=Deep Space Ice
    bool m_spiral_twist = true;

    float m_flight_z = 0.0f;
    float m_flight_speed = 28.0f;
    float m_sway_amp = 1.0f;
    float m_time = 0.0f;
};

} // namespace demoscene
