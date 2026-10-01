#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <vector>
#include <euler/vector/vector.hh>

namespace demoscene {

/**
 * @brief Real-Time 2D Phong Bump Mapping and Embossed Surface Lighting.
 *
 * Origin: Classic 1990s demoscene effect (retro-demoeffects / oldskewlish).
 *
 * Demonstrates:
 * - Surface gradient normal vectors: ∇H = (∂H/∂x, ∂H/∂y).
 * - Precomputed 256x256 Phong specular/diffuse spherical lightmap lookup.
 * - Dynamic moving light source with specular sheen and drop shadows.
 * - Multiple procedural bump heightmaps (Demoscene Emblem, Cyber Circuit, Greek Maze).
 */
class bump_effect final : public effect_base {
public:
    bump_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "2D Phong Bump Mapping";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "bump.cpp & bump2d (retro-demoeffects)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Johan Gardhage";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "I(x,y) = LightMap(L_x - x + k·∇H_x, L_y - y + k·∇H_y)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Real-time 2D Phong bump mapping with normal gradient displacement and dynamic specular spotlight.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Arrows: Move Light | Space: Heightmap (Emblem, Circuit, Maze) | C: Theme | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();
    void generate_lightmap();
    void generate_heightmap(int map_idx);

    vga_canvas m_canvas;

    std::vector<uint8_t> m_heightmap;
    std::vector<uint8_t> m_lightmap;

    float m_light_x = 160.0f;
    float m_light_y = 100.0f;
    float m_time = 0.0f;
    bool m_manual_control = false;

    int m_map_idx = 0;      // 0 = Demoscene Emblem, 1 = Cyber Circuit, 2 = Greek Maze
    int m_color_theme = 0;  // 0 = Polished Copper & Gold, 1 = Synthwave Violet, 2 = Silver Steel
};

} // namespace demoscene
