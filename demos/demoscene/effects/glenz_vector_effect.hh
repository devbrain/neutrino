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
 * @brief 3D Glenz Vector Polyhedron with Additive Translucent Facets.
 *
 * Origin: Classic Amiga Megademo (Red Sector) & glenzcube.cpp by Johan Gardhage.
 *
 * Demonstrates:
 * - 3D rotating translucent polyhedron rendered with both front and back faces.
 * - Additive blending per rasterized pixel: framebuffer[y, x] += color.
 * - Overlapping crystal faces create luminous internal seams and gem-like refraction.
 * - 3D quaternion rotation via euler::quaternion<float> with hoisted matrix transforms.
 * - Multiple polyhedra: Glenz Cube, Diamond Octahedron, and Stella Octangula (Star).
 */
class glenz_vector_effect final : public effect_base {
public:
    glenz_vector_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "3D Glenz Vector Polyhedron";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "glenzcube.cpp (Johan Gardhage & Red Sector Amiga)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Red Sector & Johan Gardhage";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "P' = M·P;  fb[y, x] = min(255, fb[y, x] + facet_intensity)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Translucent 3D crystal polyhedron with additive blending revealing internal geometry and overlapping facet seams.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Polyhedron Type | C: Palette Theme | Arrows: Rotate 3D | W/S: Zoom | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

    enum class PolyType {
        Cube = 0,
        Octahedron,
        StarOctangula,
        Count
    };

private:
    struct Face {
        std::vector<int> vertex_indices;
        uint8_t base_color;
    };

    struct Model {
        std::vector<euler::vec3<float>> vertices;
        std::vector<Face> faces;
    };

    void init_palette();
    void build_models();
    void draw_glenz_polygon(const std::vector<std::pair<int, int>>& pts, uint8_t color, uint8_t* raw);

    vga_canvas m_canvas;

    std::vector<Model> m_models;
    PolyType m_poly_type = PolyType::Cube;
    int m_theme = 0; // 0=Ruby Crystal, 1=Emerald Gem, 2=Sapphire Neon, 3=Amethyst Violet

    euler::quaternion<float> m_orientation{euler::quaternion<float>::identity()};
    euler::vec3<float> m_angular_velocity{0.6f, 0.9f, 0.4f};
    float m_zoom = 160.0f;
    float m_time = 0.0f;
};

} // namespace demoscene
