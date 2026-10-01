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
 * @brief Reconstruction of Bas van Gaalen's 3D Rotating Checkerboard (3DCHKBRD.PAS).
 *
 * Demonstrates:
 * - 3D parametric surface mesh (36 vertices, 25 quads, alternating checked tiles).
 * - Multi-axis quaternion rotation eliminating gimbal lock (euler::quaternion<float>).
 * - Dynamic undulating surface wave displacement: z(u, v, t) = A·sin(ku·u + t)·cos(kv·v + t).
 * - Depth sorting (Painter's Algorithm) and Lambertian diffuse lighting with face normals.
 */
class checkerboard3d_effect final : public effect_base {
public:
    checkerboard3d_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "3D Checkerboard Plane";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "3DCHKBRD.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen & Sean Palmer";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "v' = q · (u, v, z) · q⁻¹; z = A · sin(ω₁u + t) · cos(ω₂v + t)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Tumbling 3D checkerboard surface with dynamic undulating ripples and diffuse lighting.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Arrows: Rotate | W/S: Zoom | Space: Wave Mode | C: Color Theme | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    struct quad_face {
        size_t v0, v1, v2, v3;
        bool is_white;
        float depth = 0.0f;
    };

    void init_palette();
    void build_mesh();
    void rasterize_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color);

    vga_canvas m_canvas;

    static constexpr int grid_size = 7; // 7x7 vertices = 6x6 = 36 quad tiles
    static constexpr int vertex_count = grid_size * grid_size;

    std::vector<euler::vec3<float>> m_base_vertices;
    std::vector<quad_face> m_faces;

    euler::quaternion<float> m_orientation;
    euler::vec3<float> m_angular_velocity{0.6f, 0.9f, 0.4f};
    float m_zoom = 220.0f;
    float m_time = 0.0f;

    int m_wave_mode = 1;     // 0 = Flat Rigid Plane, 1 = Undulating Ripple Wave, 2 = Hyperbolic Twist
    int m_color_theme = 0;   // 0 = Classic B&W / Amber, 1 = Cyberpunk Cyan/Magenta, 2 = Emerald & Gold
};

} // namespace demoscene
