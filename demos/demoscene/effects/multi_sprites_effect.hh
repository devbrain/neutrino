#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <vector>

namespace demoscene {

/**
 * @brief 100 Additive Transparent Bobs with Bit-Mask Palette Blending.
 *
 * Origin: SPRITES2.PAS by Bas van Gaalen & David Dahl (Feb 14, 1995).
 *
 * Demonstrates:
 * - 1990s demoscene hardware-free additive transparency on 256-color paletted VGA.
 * - Bit-mask channel decomposition: Red (bit 3 / +8), Green (bit 4 / +16), Blue (bit 5 / +32).
 * - Optical additive color mixing: R|G -> Yellow, R|B -> Magenta, G|B -> Cyan, R|G|B -> White.
 * - Bit 7 (128) backdrop translucent tinting layer.
 * - 100 concurrent Lissajous orbit trajectories: x(t) = sin(t)+cos(2t), y(t) = cos(t)-sin(2t).
 */
class multi_sprites_effect final : public effect_base {
public:
    multi_sprites_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "100 Additive Transparent Sprites";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "SPRITES2.PAS (Bas van Gaalen, 1995)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen & David Dahl";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "P = P_bg | S_rgb;  R|G=Yellow, R|B=Magenta, G|B=Cyan, R|G|B=White";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "100 bouncing translucent RGB sprites using bitwise additive palette combining over textured backdrop.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Swarm Formation | +/-: Sprite Count | B: Backdrop Mode | C: Palette | R: Reset";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

private:
    void init_palette();
    void init_bobs();
    void init_background();
    void init_tables();

    vga_canvas m_canvas;

    using bob_template = std::array<uint8_t, 64>;
    std::array<bob_template, 3> m_bobs{};

    std::array<int, 256> m_stab1{};
    std::array<int, 256> m_stab2{};

    struct sprite_state {
        int x;
        int y;
        uint8_t idx1;
        uint8_t idx2;
        uint8_t bob_type;
    };

    static constexpr size_t max_sprites = 150;
    std::vector<sprite_state> m_sprites;
    size_t m_active_sprites = 100;

    std::vector<uint8_t> m_background;

    int m_formation = 0;   // 0 = Authentic Lissajous Swarm, 1 = Gravitational Vortex, 2 = Sine Wave Ribbon
    int m_backdrop_mode = 0; // 0 = Authentic Split Screen, 1 = Checkerboard, 2 = Pure Black
    int m_palette_theme = 0; // 0 = Authentic RGB Additive, 1 = Pastel CMY, 2 = Neon Cyber
    float m_time = 0.0f;
};

} // namespace demoscene
