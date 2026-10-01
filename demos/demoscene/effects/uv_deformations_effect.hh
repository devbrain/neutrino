#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <vector>

namespace demoscene {

/**
 * @brief Canonical Procedural UV Deformations & Fly-Throughs.
 *
 * Origin: uvmapgen.h (Pierre-Jean Turpeau / oldskewlish & Iñigo Quílez).
 *
 * Demonstrates:
 * - 4 canonical demoscene procedural UV coordinate transformations:
 *   1. Dual Symmetric Planes: Flying between infinite parallel floor and ceiling planes (u = x/|y|, v = 1/|y|).
 *   2. Logarithmic Swirl Tunnel: Polar spiral vortex with harmonic twisting.
 *   3. Concentric Harmonic Waves: Radial ripple wave mapping (v = r + r·sin(4r)).
 *   4. TV CRT Barrel Warp: Non-linear magnetic tube pincushion curvature and edge vignette.
 * - Distance fog attenuation and texture repeat wrapping.
 */
class uv_deformations_effect final : public effect_base {
public:
    uv_deformations_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Procedural UV Deformations";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "uvmapgen.h (P-J Turpeau & Iñigo Quílez)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Iñigo Quílez & Pierre-Jean Turpeau";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "u = x / |y|;  v = 1 / |y|;  c = tex[(u + t) & 63, (v + t) & 63] · fog(y)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Canonical 2D procedural UV coordinate transformations: Dual Infinite Planes, Swirl Tunnel, Waves, and CRT Barrel.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Deformation Type | T: Texture | +/-: Fly Speed | Arrows: Pan/Tilt | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

    enum class DeformType {
        SymmetricPlanes = 0,
        SwirlTunnel,
        ConcentricWaves,
        TvBarrelWarp,
        Count
    };

private:
    static constexpr int tex_size = 64;

    void init_palette();
    void generate_textures();

    vga_canvas m_canvas;

    static constexpr int num_textures = 3;
    std::array<std::array<uint8_t, tex_size * tex_size>, num_textures> m_textures{};

    DeformType m_deform_type = DeformType::SymmetricPlanes;
    int m_texture_idx = 0;

    float m_scroll_u = 0.0f;
    float m_scroll_v = 0.0f;
    float m_fly_speed = 1.0f;
    float m_pan_x = 0.0f;
    float m_pan_y = 0.0f;
    float m_time = 0.0f;
};

} // namespace demoscene
