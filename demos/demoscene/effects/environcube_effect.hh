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
 * @brief Environment-Mapped 3D Chrome Polyhedra (Spherical Reflection Mapping).
 *
 * Origin: Classic 1990s demoscene effect (retro-demoeffects / environcube.cpp).
 *
 * Demonstrates:
 * - Spherical environment reflection mapping: R = 2(N·V)N - V; (u, v) = R_{x,y}/2 + 0.5.
 * - Multi-axis quaternion 3D rotation via euler::quaternion<float>.
 * - Perspective texture-mapped triangle rasterization across 3D polyhedra.
 * - Procedural chrome studio environment maps (Liquid Chrome, Sunset Horizon, Cyber Neon).
 */
class environcube_effect final : public effect_base {
public:
    environcube_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Environment-Mapped 3D Cube";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "environcube.cpp (retro-demoeffects)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Johan Gardhage";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "R = 2(N·V)N - V; u = R_x/2 + ½, v = R_y/2 + ½";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "3D polyhedra rotating with quaternions, texture-mapped with real-time spherical chrome environment reflections.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Shape (Cube, Octahedron, Torus) | C: EnvMap | Arrows: Rotate | W/S: Zoom | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    struct tri_face {
        size_t v0, v1, v2;
        float depth = 0.0f;
    };

    void init_palette();
    void generate_envmap(int theme_idx);
    void build_shape(int shape_idx);
    void rasterize_env_triangle(int x0, int y0, float u0, float v0,
                                int x1, int y1, float u1, float v1,
                                int x2, int y2, float u2, float v2);

    vga_canvas m_canvas;

    std::vector<euler::vec3<float>> m_vertices;
    std::vector<tri_face> m_faces;
    std::vector<uint8_t> m_envmap;

    euler::quaternion<float> m_orientation;
    euler::vec3<float> m_angular_velocity{0.5f, 0.8f, 0.3f};
    float m_zoom = 240.0f;
    float m_time = 0.0f;

    int m_shape_idx = 0;   // 0 = Chrome Cube, 1 = Diamond Octahedron, 2 = Hex Torus
    int m_theme_idx = 0;   // 0 = Studio Chrome, 1 = Sunset Horizon, 2 = Cyber Neon
};

} // namespace demoscene
