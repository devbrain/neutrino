#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <vector>

namespace demoscene {

/**
 * @brief 2D Sine Mesh Texture Distortion & Rubber-Sheet Deform.
 *
 * Origin: distort.cpp & distortlogo.cpp by Johan Gardhage (Demoresearch).
 *
 * Demonstrates:
 * - Real-time 2D non-linear displacement coordinate mapping: (x', y') = (x + dx(x, y, t), y + dy(x, y, t)).
 * - 4 distortion engines:
 *   1. Liquid Rubber Sheet (authentic compound harmonic field from distort.cpp).
 *   2. Underwater Flag Ripple (drifting horizontal and vertical sine waves).
 *   3. Swirling Polar Vortex (rotational twist around moving focal center).
 *   4. CRT Barrel & TV Sync Wobble (scanline jitter and magnetic tube distortion).
 * - Rich procedural demoscene background scenes.
 */
class mesh_distort_effect final : public effect_base {
public:
    mesh_distort_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "2D Mesh Texture Distortion";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "distort.cpp (Johan Gardhage)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Johan Gardhage";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "x' = x + Δx(x,y,t);  y' = y + Δy(x,y,t);  fb[y, x] = src[y', x']";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Full-screen non-linear 2D coordinate displacement warping procedural scenes like liquid rubber.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Distort Engine | M: Scene Image | +/-: Amplitude | Arrows: Frequency | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

    enum class DistortMode {
        LiquidRubber = 0,
        UnderwaterFlag,
        PolarVortex,
        CrtWobble,
        Count
    };

private:
    void init_palette();
    void render_source_scene(int scene_id, uint8_t* dst);

    vga_canvas m_canvas;

    // Internal 320x200 pristine source scene buffer
    std::array<uint8_t, vga_canvas::pixel_count> m_source_buffer{};

    DistortMode m_distort_mode = DistortMode::LiquidRubber;
    int m_scene_id = 0; // 0=Copper Logo, 1=Checker Horizon, 2=Cyber Grid

    float m_amplitude = 18.0f;
    float m_frequency = 1.0f;
    float m_time = 0.0f;
};

} // namespace demoscene
