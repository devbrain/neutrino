#pragma once

#include "effect_base.hh"

#include <vector>
#include <euler/angles/degree.hh>
#include <euler/quaternion/quaternion.hh>
#include <euler/quaternion/quaternion_ops.hh>
#include <euler/vector/vector.hh>
#include <euler/vector/vector_ops.hh>
#include <sdlpp/video/color.hh>

namespace demoscene {

/**
 * @brief Reconstruction of Bas van Gaalen's 3D Rotation routines (ROTATE1.PAS, ROT3.PAS).
 *
 * Demonstrates:
 * - 3D orientation tracking using euler::quaternion<float> (eliminating gimbal lock).
 * - Perspective divide: x' = x·d / (z + z0).
 * - Vector mathematics via euler::vec3<float> for backface culling (dot product with camera ray).
 * - Sub-pixel anti-aliased vector rendering using neutrino::draw_line_aa().
 */
class rotate3d_effect final : public effect_base {
public:
    rotate3d_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "3D Quaternion Polyhedra";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "ROTATE1.PAS & ROT3.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "v' = q · v · q⁻¹; x' = xc + x·D / (z + z0)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Multi-axis 3D solid rotation using unit quaternions and anti-aliased vector rasterization.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Arrows: Rotate | W/S: Zoom | M: Toggle Mesh Model | R: Reset";
    }

private:
    struct edge {
        size_t a;
        size_t b;
    };

    struct face {
        size_t v0, v1, v2;
        sdlpp::color base_color;
    };

    void build_mesh(int model_index);

    std::vector<euler::vec3<float>> m_base_vertices;
    std::vector<edge> m_edges;
    std::vector<face> m_faces;

    euler::quaternion<float> m_orientation;
    euler::vec3<float> m_angular_velocity{0.5f, 0.8f, 0.3f};
    float m_zoom = 280.0f;
    int m_current_model = 0; // 0 = Octahedron, 1 = Icosahedron, 2 = Cube
};

} // namespace demoscene
