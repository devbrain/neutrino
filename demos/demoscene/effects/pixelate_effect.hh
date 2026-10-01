#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <vector>

namespace demoscene {

/**
 * @brief Macroblock Pixelation & Decimation Dissolve Transitions.
 *
 * Origin: PIXELATE.PAS by Bas van Gaalen (Holland, 1994) & Pierre-Jean Turpeau.
 *
 * Demonstrates:
 * - Macroblock decimation from fine 1x1 pixels to coarse 64x64 mosaic blocks.
 * - Seamless demoscene transitions between 4 procedural high-detail scenes:
 *   1. Copper bars with embossed demoscene logo and starfield.
 *   2. Polar psychedelic plasma / hypnotic tunnel.
 *   3. 3D perspective checkered floor with floating glass orb.
 *   4. Cosmic nebula with pulsing galactic core.
 * - Authentic top-left pixel block sampling vs centered vs area-averaged sampling.
 * - Dynamic animated pixelation breathing and discrete power-of-2 stepping (2, 4, 8, 16, 32, 64).
 */
class pixelate_effect final : public effect_base {
public:
    pixelate_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Mosaic Pixelation & Decimation";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "PIXELATE.PAS (Bas van Gaalen, 1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen & Pierre-Jean Turpeau";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "c = scene[(j·z)·W + i·z];  block[y, x] = c  (z = 1..64)";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Macroblock mosaic decimation dissolving between 4 procedural demoscene scenes with authentic 1994 VGA sampling.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Trigger Transition | M: Mode | S: Sample Method | Left/Right: Scene | Up/Down: Block Size";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

    enum class Mode {
        AutoTransition = 0,
        SineBreathe,
        SteppedPowers,
        AnisotropicDecimation,
        Count
    };

    enum class SampleMethod {
        TopLeft = 0,  // Authentic PIXELATE.PAS
        Center,
        Average,
        Count
    };

private:
    void init_palette();
    void render_scene(int scene_id, float time, uint8_t* dst);
    void apply_pixelation(const uint8_t* src, uint8_t* dst, float block_size_x, float block_size_y);

    vga_canvas m_canvas;

    // Internal full-resolution offscreen backbuffer for the source scene (320x200)
    std::array<uint8_t, vga_canvas::pixel_count> m_scene_buffer{};

    Mode m_mode = Mode::AutoTransition;
    SampleMethod m_sample_method = SampleMethod::TopLeft;

    int m_current_scene = 0;
    int m_target_scene = 1;

    // Transition state
    float m_block_size = 1.0f;
    float m_block_size_y = 1.0f;
    float m_transition_timer = 0.0f;
    bool m_transitioning = false;
    bool m_transition_swap_done = false;

    float m_time = 0.0f;
    float m_manual_size = 8.0f;
};

} // namespace demoscene
