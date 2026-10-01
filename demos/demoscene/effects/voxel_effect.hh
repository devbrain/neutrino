#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <vector>

namespace demoscene {

/**
 * @brief Reconstruction of the Comanche-style Voxel Space engine (VOXEL.PAS).
 *
 * Demonstrates:
 * - Procedural fractal heightfield (Diamond-Square / Midpoint Displacement).
 * - Front-to-back raymarched 3D terrain projection.
 * - 1D vertical occlusion horizon buffer (oldy[320]) preventing overdraw.
 * - Interactive flight controls (altitude, heading, speed).
 */
class voxel_effect final : public effect_base {
public:
    voxel_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Comanche Voxel Space";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "VOXEL.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Jeroen Bouwens & Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "y_proj = horizon + (cam_h - h_map) · scale / depth; Occlusion: y < old_y[x]";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Real-time 2.5D voxel raymarched heightfield with dynamic camera navigation.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Arrows: Altitude & Heading | W/S: Speed | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void generate_heightmap();
    [[nodiscard]] float sample_height(float x, float y) const noexcept;

    static constexpr int map_size = 256;
    static constexpr int map_mask = map_size - 1;

    vga_canvas m_canvas;
    std::vector<uint8_t> m_heightmap;

    float m_cam_x = 128.0f;
    float m_cam_y = 0.0f;
    float m_cam_altitude = 35.0f;
    float m_cam_angle = 1.5707963f; // pi / 2 (facing +Y)
    float m_speed = 45.0f;
};

} // namespace demoscene
