#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <vector>
#include <euler/vector/vector.hh>

namespace demoscene {

/**
 * @brief Textured 3D Cylinder Tunnel with Lissajous Flight and Depth Fog.
 *
 * Origin: Classic 1990s demoscene effect (retro-demoeffects & oldskewlish).
 *
 * Demonstrates:
 * - Cylindrical polar texture mapping: u = (θ / π) · W_tex, v = (D / r) + speed·t.
 * - Dynamic Lissajous camera sway simulating 3D flight through a winding tube.
 * - 1/z distance attenuation and exponential depth fog.
 * - Seamless 256x256 procedural textures (Checkered Stone, Cyber Hex, Organic Ribs).
 */
class rototunnel_effect final : public effect_base {
public:
    rototunnel_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Textured 3D Cylinder Tunnel";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "rototunnel.cpp (retro-demoeffects)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Johan Gardhage";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "u = (atan2(dy, dx)/π)·W + roll·t; v = (D / √(dx²+dy²)) + spd·t";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Polar coordinate textured 3D cylinder tunnel with Lissajous camera sway and depth fog.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Pattern (Stone, Hex, Organic) | C: Theme | Up/Down: Speed | Left/Right: Roll | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();
    void generate_texture(int pattern_idx);

    vga_canvas m_canvas;

    std::vector<uint8_t> m_texture;

    float m_cam_x = 160.0f;
    float m_cam_y = 100.0f;
    float m_speed = 120.0f;
    float m_roll_speed = 0.5f;
    float m_tunnel_pos = 0.0f;
    float m_tunnel_rot = 0.0f;
    float m_time = 0.0f;

    int m_pattern_idx = 0; // 0 = Stone Checker, 1 = Cyber Hex, 2 = Organic Ribs
    int m_color_theme = 0; // 0 = Amber / Flame, 1 = Cyan / Azure, 2 = Toxic Neon
};

} // namespace demoscene
