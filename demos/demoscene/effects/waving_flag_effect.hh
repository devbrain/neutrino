#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <vector>

namespace demoscene {

/**
 * @brief 3D Waving Cloth & Dot Flag Physics Simulator.
 *
 * Origin: dotflag.cpp by Johan Gardhage (Demoresearch).
 *
 * Demonstrates:
 * - 4,000-node 2D/3D lattice mesh simulating cloth aerodynamics and billowing wind.
 * - Compound traveling wave equations across u and v: z = A·sin(k1·u - ω·t) + B·cos(k2·v + φ).
 * - Specular crest highlighting and perspective 3D depth cues.
 * - 4 flag patterns: Swedish Flag (original homage), Jolly Roger, Demoscene Banner, Holographic Pride.
 */
class waving_flag_effect final : public effect_base {
public:
    waving_flag_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "3D Waving Cloth & Dot Flag";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "dotflag.cpp (Johan Gardhage)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Johan Gardhage";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "z = A·sin(k1·u - ω·t) + B·cos(k2·v + φ);  I_col = col_base + crest_specular";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "4,000-node particle cloth lattice billowing in turbulent wind with specular wave crests.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Flag Pattern | Arrows: Wind Speed/Angle | +/-: Wave Amplitude | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    static constexpr int flag_cols = 80;
    static constexpr int flag_rows = 50;
    static constexpr int num_particles = flag_cols * flag_rows;

    struct Particle {
        float base_x;
        float base_y;
        uint8_t color[4]; // Colors for 4 flag themes
    };

    void init_palette();
    void build_flag();

    vga_canvas m_canvas;
    std::vector<Particle> m_particles;

    int m_flag_theme = 0; // 0=Swedish Flag, 1=Jolly Roger, 2=Demoscene, 3=Holographic
    float m_wave_amp = 22.0f;
    float m_wind_speed = 3.5f;
    float m_time = 0.0f;
};

} // namespace demoscene
