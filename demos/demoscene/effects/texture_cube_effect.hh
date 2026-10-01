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
 * @brief 3D Affine Texture-Mapped Polygon Cube with Dynamic Lighting.
 *
 * Origin: 3D_TMAP2.PAS by Jeroen Bouwens & Bas van Gaalen (1994) & texturecube.cpp.
 *
 * Demonstrates:
 * - Real-time affine texture mapping across 12 triangles forming a 3D solid cube.
 * - Backface culling via 2D screen cross product: (x1 - x0)(y2 - y0) - (y1 - y0)(x2 - x0).
 * - Scanline affine texture coordinate interpolation (du, dv).
 * - Directional Lambertian face lighting: I = ambient + diffuse · (N · L).
 * - 3 procedural 64x64 textures: Demoscene Checkerboard, Cyber Grid, and Aztec Gold.
 */
class texture_cube_effect final : public effect_base {
public:
    texture_cube_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "3D Texture-Mapped Cube";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "3D_TMAP2.PAS (Jeroen Bouwens & Bas van Gaalen, 1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Jeroen Bouwens & Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "u(x) = u_L + (x - x_L)·du;  v(x) = v_L + (x - x_L)·dv;  c = tex[u & 63, v & 63]";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Solid 3D cube with affine texture-mapped triangular faces, backface culling, and Lambertian lighting.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Texture Theme | L: Toggle Lighting | Arrows: Rotate 3D | W/S: Zoom | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    struct VertexUV {
        euler::vec3<float> pos;
        float u;
        float v;
    };

    struct Triangle {
        int v0, v1, v2;
        euler::vec3<float> normal;
    };

    static constexpr int tex_size = 64;

    void init_palette();
    void generate_textures();
    void draw_textured_triangle(
        float x0, float y0, float u0, float v0,
        float x1, float y1, float u1, float v1,
        float x2, float y2, float u2, float v2,
        float light_intensity, uint8_t* raw);

    vga_canvas m_canvas;

    std::vector<VertexUV> m_vertices;
    std::vector<Triangle> m_triangles;

    // 3 procedural 64x64 textures
    static constexpr int num_textures = 3;
    std::array<std::array<uint8_t, tex_size * tex_size>, num_textures> m_textures{};

    int m_texture_idx = 0;
    bool m_enable_lighting = true;

    euler::quaternion<float> m_orientation{euler::quaternion<float>::identity()};
    euler::vec3<float> m_angular_velocity{0.7f, 1.1f, 0.45f};
    float m_zoom = 150.0f;
    float m_time = 0.0f;
};

} // namespace demoscene
