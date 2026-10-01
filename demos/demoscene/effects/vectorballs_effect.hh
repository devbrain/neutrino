#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <vector>
#include <euler/angles/radian.hh>
#include <euler/quaternion/quaternion.hh>
#include <euler/quaternion/quaternion_ops.hh>
#include <euler/vector/vector.hh>
#include <euler/vector/vector_ops.hh>

namespace demoscene {

/**
 * @brief Reconstruction and expansion of Bas van Gaalen's Vectorballs (BOPS.PAS).
 *
 * Demonstrates:
 * - 3D vectorball structures (rotating cube, double helix, torus knot) rotated via euler::quaternion<float>.
 * - Painter's Algorithm depth sorting for correct ball occlusion.
 * - Perspective 1/z ball radius scaling and 3D spherical Lambertian + specular shading.
 * - Dynamic color themes and interactive orbital controls.
 */
class vectorballs_effect final : public effect_base {
public:
    vectorballs_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "3D Vectorballs";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "BOPS.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "P'_i = q · P_i · q⁻¹; x_s = x·D/z, r_s = R·D/z";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Classic 3D vectorballs rotating with quaternions, depth sorting, dynamic radius scaling, and specular highlights.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Shape (Cube, Double Helix, Torus) | C: Theme | Arrows: Rotate | W/S: Zoom | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    struct ball {
        euler::vec3<float> local_pos;
        float radius = 10.0f;
    };

    void init_palette();
    void build_shape(int shape_idx);
    void draw_shaded_ball(int cx, int cy, float radius, uint8_t base_col);

    vga_canvas m_canvas;

    std::vector<ball> m_balls;

    euler::quaternion<float> m_orientation;
    euler::vec3<float> m_angular_velocity{0.6f, 0.9f, 0.4f};
    float m_zoom = 220.0f;
    float m_time = 0.0f;

    int m_shape_idx = 0;   // 0 = 3D Cube (4x4x4), 1 = Double Helix, 2 = Torus Knot
    int m_color_theme = 0; // 0 = Metallic Chrome, 1 = Synthwave Magenta, 2 = Emerald Gold
};

} // namespace demoscene
