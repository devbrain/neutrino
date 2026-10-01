#pragma once

#include "effect_base.hh"
#include "../vga_canvas.hh"

#include <array>
#include <string_view>
#include <vector>

namespace demoscene {

/**
 * @brief Stretching, Wobbling, & Accordion Scanline Scroller.
 *
 * Origin: STRSCR.PAS & EFFECT4.PAS by Bas van Gaalen (Holland, 1994).
 *
 * Demonstrates:
 * - Dynamic non-linear scanline expansion using trigonometric differential table diffsin[64].
 * - Horizontal sine wobble wave displacement stab[256].
 * - Vertical continuous text buffer rasterization with BIOS 8x8 font.
 * - Horizontal pixel doubling (160x41 bitmap expanded to full 320x200 canvas).
 * - Accordion rubber-band vertical scanline pulling from EFFECT4.PAS.
 * - Layered copper bar backdrop with CRT phosphor persistence.
 */
class stretch_scroll_effect final : public effect_base {
public:
    stretch_scroll_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "Stretch & Wobble Scroller";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "STRSCR.PAS & EFFECT4.PAS (Bas van Gaalen, 1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "offset = diffsin[(y+i1+i2)&63];  pos = (add + repy + stab[(i2+add+x)&255])·320";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Non-linear scanline stretching and horizontal sine wobble scroller with rubber-band accordion distortion.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Space: Mode | B: Copper Backdrop | W: Wobble Amp | Up/Down: Stretch | Left/Right: Speed";
    }

    [[nodiscard]] const vga_canvas& canvas() const noexcept { return m_canvas; }

    enum class Mode {
        StretchAndWobble = 0,   // Authentic STRSCR.PAS
        AccordionStretcher,     // EFFECT4.PAS rubberband pull
        DualLayerRibbon,        // Layered banner + copper
        HarmonicWobble,         // Multi-frequency compound wave
        Count
    };

private:
    void init_palette();
    void init_tables();
    void render_copper_backdrop(float time);
    void update_text_raster(float delta);
    void render_stretch_scroll();
    void render_accordion_stretcher();

    vga_canvas m_canvas;

    // Authentic tables from STRSCR.PAS
    static constexpr std::array<uint8_t, 64> s_diffsin = {
        0,0,0,1,0,1,1,2,1,2,2,3,2,3,3,4,3,4,4,5,
        4,5,5,4,5,4,4,3,4,3,3,2,3,2,2,1,2,1,1,0,
        1,0,0,0,0,1,2,2,3,3,3,4,4,4,4,3,3,3,2,2,1,0,0,0
    };
    std::array<int8_t, 256> m_stab{};

    // 41 rows x 160 columns bitmap buffer from STRSCR.PAS
    static constexpr int bitmap_rows = 41;
    static constexpr int bitmap_cols = 160;
    std::array<std::array<uint8_t, bitmap_cols>, bitmap_rows> m_bitmap{};

    // Effect4 logo buffer (102 x 32)
    static constexpr int logo_w = 102;
    static constexpr int logo_h = 32;
    std::array<uint8_t, logo_w * logo_h> m_logo_pixels{};

    Mode m_mode = Mode::StretchAndWobble;
    bool m_show_backdrop = true;
    float m_wobble_scale = 1.0f;
    float m_stretch_mult = 1.0f;
    float m_scroll_speed = 30.0f;

    // Animation indices from STRSCR.PAS
    int m_idx1 = 0;
    int m_idx2 = 40;
    int m_txt_line_idx = 0;
    float m_subpixel_scroll = 0.0f;
    float m_time = 0.0f;

    // Text scroll messages
    std::vector<std::string> m_messages;
};

} // namespace demoscene
