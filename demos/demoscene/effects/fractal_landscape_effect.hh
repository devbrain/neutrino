#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <vector>

namespace demoscene {

/**
 * @brief 3D Fractal Landscape Flight Simulator.
 *
 * Origin: SCAPE.PAS & SCAPE2.PAS by Bas van Gaalen (Holland, 1994).
 *
 * Demonstrates:
 * - Recursive diamond-square midpoint displacement 2D fractal heightmap generation.
 * - Real-time perspective voxel raymarch / column projection with horizon buffer occlusion.
 * - Interactive 3D flight camera with 6-DOF controls (pitch, yaw, speed, altitude).
 * - Altitude color zoning: deep water, sand shoreline, green valleys, rocky ridges, snow peaks.
 * - Distance fog atmospheric attenuation.
 */
class fractal_landscape_effect final : public effect_base {
public:
    fractal_landscape_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Fractal Landscape Flight";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "SCAPE.PAS & SCAPE2.PAS (Bas van Gaalen, 1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "y_s = y_h - (H(u,v) - z_cam)·S_z / d;  draw_col(y_s, old_y, col)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Diamond-square fractal terrain flyover with column projection, altitude zoning, and flight camera.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: New Terrain | Arrows: Pitch/Turn | W/S: Altitude | +/-: Speed | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    static constexpr int map_size = 256;
    static constexpr int map_mask = map_size - 1;

    void init_palette();
    void generate_terrain();
    void midpoint_displacement(int x0, int y0, int x1, int y1, float roughness);

    vga_canvas m_canvas;

    std::array<uint8_t, map_size * map_size> m_heightmap{};

    // Flight camera
    float m_cam_x = 0.0f;
    float m_cam_y = 0.0f;
    float m_cam_z = 95.0f;
    float m_cam_yaw = 0.0f;
    float m_cam_pitch = 0.0f;
    float m_speed = 35.0f;
    float m_time = 0.0f;

    uint32_t m_rng_state = 123456789;
    uint32_t rand_u32();
    float rand_f();
};

} // namespace demoscene
