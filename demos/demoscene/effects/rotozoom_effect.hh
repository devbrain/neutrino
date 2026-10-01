#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <vector>
#include <euler/vector/vector.hh>
#include <euler/angles/radian.hh>

namespace demoscene {

/**
 * @brief Affine Texture Rotozoomer and Infinite Plane Rotation.
 *
 * Origin: Iconic 1990s demoscene effect (retro-demoeffects / oldskewlish).
 *
 * Demonstrates:
 * - 2D affine coordinate transformations: (u, v) = R(θ) · (x, y) / zoom + offset.
 * - Constant differential scanline stepping (Δu_x, Δv_x) for ultra-fast texture sampling.
 * - Procedural 256x256 seamless power-of-two tiled textures.
 * - Dynamic continuous rotation, multi-scale harmonic zoom, and panning.
 */
class rotozoom_effect final : public effect_base {
public:
    rotozoom_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Texture Rotozoomer";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "rotozoom.cpp (retro-demoeffects & oldskewlish)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Johan Gardhage & Pierre-Jean Turpeau";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "[u, v]ᵀ = (1/z)·R(θ)·[x - x_c, y - y_c]ᵀ + [u₀, v₀]ᵀ";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Infinite plane affine texture rotozoomer with constant-differential scanline stepping and procedural textures.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Left/Right: Spin | Up/Down: Zoom | Space: Texture | C: Theme | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();
    void generate_texture(int pattern_idx);

    vga_canvas m_canvas;

    std::vector<uint8_t> m_texture;

    float m_angle = 0.0f;
    float m_rot_speed = 0.8f;
    float m_zoom = 1.0f;
    float m_pos_u = 0.0f;
    float m_pos_v = 0.0f;
    float m_time = 0.0f;

    int m_pattern_idx = 0; // 0 = Checker Rosette, 1 = Cyber Circuit, 2 = Geometric Labyrinth
    int m_color_theme = 0; // 0 = Copper Spectrum, 1 = Synthwave Neon, 2 = Electric Lime
};

} // namespace demoscene
