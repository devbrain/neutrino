#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <vector>
#include <array>
#include <euler/vector/vector.hh>
#include <euler/quaternion/quaternion.hh>
#include <euler/quaternion/quaternion_ops.hh>

namespace demoscene {

/**
 * @brief 3D Rotating Gravitational Potential Well & Accretion Vortex.
 *
 * Origin: ROT7.PAS by Bas van Gaalen (Holland, June 22, 1994).
 *
 * Demonstrates:
 * - 3D warped gravitational spacetime funnel mesh and discrete potential well.
 * - 3-axis Euler and quaternion rotational kinematics using euler::quaternion<float>.
 * - Perspective depth projection: Xp = (Xc·Z - X·Zc) / (Z - Zc) + center.
 * - Keplerian particle accretion disk orbiting the central singularity.
 * - Authentic 1994 VGA DAC palette with cyan/emerald depth grading.
 */
class gravitational_well_effect final : public effect_base {
public:
    gravitational_well_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "3D Gravitational Well";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "ROT7.PAS (Bas van Gaalen, 1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "z = -M / √(x²+y²);  Xp = (Xc·Z - X·Zc) / (Z - Zc)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "3D warped gravitational potential well and orbiting accretion disk with perspective transformation.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Display Mode | Arrows: Orbit/Pitch | W/S: Zoom | Up/Down: Depth | C: Theme | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();
    void init_grid();
    void init_particles();

    vga_canvas m_canvas;

    static constexpr int grid_size = 13;
    static constexpr int total_grid_points = grid_size * grid_size; // 169 points
    std::array<euler::vec3<float>, total_grid_points> m_base_points{};

    struct particle {
        float radius;
        float angle;
        float speed;
        float z_offset;
        uint8_t color;
    };
    static constexpr size_t particle_count = 160;
    std::vector<particle> m_particles;

    int m_mode = 0;          // 0 = Authentic Points, 1 = Wireframe Mesh, 2 = Gravitational Vortex + Accretion Disk
    int m_theme = 0;         // 0 = Emerald Cyan (Authentic 1994), 1 = Event Horizon Violet, 2 = Solar Flare
    float m_well_depth = 1.0f;
    float m_zoom = 140.0f;
    float m_time = 0.0f;

    euler::quaternion<float> m_orientation{euler::quaternion<float>::identity()};
    euler::vec3<float> m_angular_velocity{-0.25f, 0.5f, 0.25f};
};

} // namespace demoscene
