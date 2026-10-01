#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <vector>
#include <euler/vector/vector.hh>
#include <euler/quaternion/quaternion.hh>
#include <euler/quaternion/quaternion_ops.hh>

namespace demoscene {

/**
 * @brief 3D Particle Topological Morphing & Dynamic Surface Interpolation.
 *
 * Origin: Classic demoscene effect (Johan Gardhage / retro-demoeffects, Future Crew).
 *
 * Demonstrates:
 * - 3D vertices smoothly morphing between multiple parametric topological manifolds
 *   (Sphere, Torus, Cube, Double Helix, Trefoil Knot).
 * - Smooth Hermite / cosine interpolation: P_i(t) = lerp(A_i, B_i, smoothstep(alpha)).
 * - 3D quaternion rotation via euler::quaternion<float> with hoisted rotation matrix.
 * - Perspective depth projection and depth-cued phosphor glow.
 */
class dotmorph_effect final : public effect_base {
public:
    struct point3f {
        float x;
        float y;
        float z;
    };

    dotmorph_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "3D Particle Morphing";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "dotmorph.cpp (Johan Gardhage, 1990s Demoresearch)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Johan Gardhage";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "P_i(t) = (1 - alpha(t))·A_i + alpha(t)·B_i;  alpha(t) = 1/2(1 - cos(pi·t))";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "3D points morphing smoothly between Sphere, Torus, Cube, Double Helix, and Trefoil Knot.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Morph Next | +/-: Points (1k..4k) | [/]: Speed | Arrows: Rotate | W/S: Zoom | A: Phosphor Trails";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();
    void generate_shapes();

    vga_canvas m_canvas;

    static constexpr size_t max_particles = 4096;
    enum shape_type { SHAPE_SPHERE = 0, SHAPE_TORUS, SHAPE_CUBE, SHAPE_HELIX, SHAPE_TREFOIL, SHAPE_COUNT };

    std::vector<std::vector<point3f>> m_shapes;

    size_t m_active_particles = 2048;
    int m_source_shape = 0;
    int m_target_shape = 1;
    float m_morph_progress = 0.0f; // 0.0 -> 2.0 (0..1 morphing, 1..2 holding)
    float m_morph_speed = 1.2f;
    bool m_auto_morph = true;
    bool m_phosphor_trails = false;

    int m_theme = 0; // 0=Chrome Cyan, 1=Gold Amber, 2=Neon Magenta, 3=Emerald Matrix
    euler::quaternion<float> m_orientation{euler::quaternion<float>::identity()};
    euler::vec3<float> m_angular_velocity{0.5f, 1.1f, 0.35f};
    float m_zoom = 150.0f;
    float m_time = 0.0f;
};

} // namespace demoscene
