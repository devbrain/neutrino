#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <vector>
#include <euler/vector/vector.hh>
#include <euler/quaternion/quaternion.hh>
#include <euler/quaternion/quaternion_ops.hh>

namespace demoscene {

/**
 * @brief 3D Bouncing & Rotating Wireframe Sphere with Parabolic Trajectory.
 *
 * Origin: ROT10.PAS by Bas van Gaalen (Holland, 1994).
 *
 * Demonstrates:
 * - 3D rotating spherical point cloud & latitude/longitude wireframe rings.
 * - Authentic 1994 parabolic gravity bounce table ptab[256].
 * - Wall-to-wall horizontal bounding kinematics with floor bounce.
 * - 3D quaternion rotation via euler::quaternion<float> with hoisted matrix transforms.
 * - Depth-cued perspective projection: Xp = obj_x + (-x·dist)/(z - dist).
 */
class bouncing_sphere3d_effect final : public effect_base {
public:
    bouncing_sphere3d_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "3D Bouncing & Rotating Sphere";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "ROT10.PAS (Bas van Gaalen, 1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "Xp = obj_x + (-x·D)/(z-D);  Yp = 50 + ptab[pc] + (-y·D)/(z-D)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "3D sphere bouncing off walls and floor along a parabolic gravity trajectory with 3-axis rotation.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Sphere Mode | Arrows: Pitch/Yaw | +/-: Bounce Speed | C: Theme | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();
    void init_points();

    vga_canvas m_canvas;

    static constexpr size_t dots_count = 100;
    std::array<euler::vec3<float>, dots_count> m_dots{};

    int m_mode = 0;          // 0 = Authentic Points, 1 = Wireframe Rings, 2 = Multi-Sphere Cascade
    int m_theme = 0;         // 0 = Authentic Copper/Cyan, 1 = Emerald Neon, 2 = Magma Amber

    float m_obj_x = 160.0f;
    float m_x_vel = 75.0f;
    float m_pc = 128.0f;
    float m_bounce_speed = 1.0f;

    euler::quaternion<float> m_orientation{euler::quaternion<float>::identity()};
    euler::vec3<float> m_angular_velocity{0.8f, 1.2f, -0.6f};
    float m_time = 0.0f;
};

} // namespace demoscene
