#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <vector>
#include <euler/angles/radian.hh>
#include <euler/quaternion/quaternion.hh>
#include <euler/quaternion/quaternion_ops.hh>
#include <euler/vector/vector.hh>
#include <euler/vector/vector_ops.hh>

namespace demoscene {

/**
 * @brief Reconstruction of Bas van Gaalen's 3D Translucent Glass Polyhedra (3D_TRANS.PAS / 3D_HOLE.PAS).
 *
 * Demonstrates:
 * - Additive translucent polygon rasterization (mem[es:di] += color) simulating tinted glass.
 * - Multi-axis quaternion orientation tracking via euler::quaternion<float>.
 * - Depth sorting (Painter's Algorithm) and face normal calculation via euler::cross().
 * - Specular edge highlights and glowing internal refraction layers.
 */
class trans_glass3d_effect final : public effect_base {
public:
    trans_glass3d_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "3D Translucent Glass";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "3D_TRANS.PAS & 3D_HOLE.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "I(x, y) = min(255, I + I_poly); v' = q · v · q⁻¹";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Additive translucent glass polyhedra with depth sorting, glowing edge highlights, and specular shine.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Arrows: Rotate | W/S: Zoom | Space: Shape (Cube, Octahedron, Torus) | C: Tint | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    struct glass_face {
        size_t v0, v1, v2;
        float depth = 0.0f;
    };

    void init_palette();
    void build_shape(int shape_idx);
    void rasterize_glass_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t tint_val);

    vga_canvas m_canvas;

    std::vector<euler::vec3<float>> m_vertices;
    std::vector<glass_face> m_faces;

    euler::quaternion<float> m_orientation;
    euler::vec3<float> m_angular_velocity{0.5f, 0.8f, 0.3f};
    float m_zoom = 240.0f;
    float m_time = 0.0f;

    int m_shape_mode = 0;   // 0 = Glass Cube, 1 = Diamond Octahedron, 2 = Hollow Hex Torus
    int m_tint_theme = 0;   // 0 = Emerald Cyan Glass, 1 = Amethyst Ruby Glass, 2 = Amber Sun Glass
};

} // namespace demoscene
