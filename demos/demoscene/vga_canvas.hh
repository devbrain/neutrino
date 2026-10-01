#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <neutrino/video/globals.hh>
#include <neutrino/video/geometry_types.hh>
#include <sdlpp/video/color.hh>
#include <sdlpp/video/texture.hh>

namespace demoscene {

/**
 * @brief High-performance C++20 modern emulation of the classic MS-DOS VGA Mode 13h ($A000:0000).
 *
 * Provides:
 * - 320x200 8-bit indexed framebuffer (64,000 bytes linear array).
 * - 256-color DAC palette registers with 6-bit DAC (0..63) to 8-bit (0..255) conversion.
 * - In-place palette cycling for demoscene plasma and copper bar animations.
 * - Single-pass palette lookup and upload to an SDL streaming texture for GPU presentation.
 */
class vga_canvas {
public:
    static constexpr int width = 320;
    static constexpr int height = 200;
    static constexpr int pixel_count = width * height;

    vga_canvas();
    ~vga_canvas() = default;

    vga_canvas(const vga_canvas&) = delete;
    vga_canvas& operator=(const vga_canvas&) = delete;
    vga_canvas(vga_canvas&&) noexcept = default;
    vga_canvas& operator=(vga_canvas&&) noexcept = default;

    // ------------------------------------------------------------------------
    // Palette Operations (DAC Emulation)
    // ------------------------------------------------------------------------

    /**
     * @brief Set DAC palette register using DOS 6-bit color intensity (0..63).
     * Automatically scales [0..63] to modern [0..255].
     */
    void set_rgb_6bit(uint8_t index, uint8_t r6, uint8_t g6, uint8_t b6);

    /**
     * @brief Set DAC palette register using standard 8-bit RGB color (0..255).
     */
    void set_rgb(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    /**
     * @brief Set a specific palette index to a color.
     */
    void set_color(uint8_t index, const sdlpp::color& color);

    /**
     * @brief Get color at palette index.
     */
    [[nodiscard]] const sdlpp::color& get_color(uint8_t index) const noexcept {
        return m_palette[index];
    }

    /**
     * @brief Rotate a range of palette entries [start_index, start_index + count).
     * @param start_index First palette entry to cycle.
     * @param count Number of entries in cycle loop.
     * @param step Number of positions to shift (positive = forward, negative = backward).
     */
    void cycle_palette(uint8_t start_index, uint8_t count, int step = 1);

    /**
     * @brief Setup default standard 256-color grayscale or rainbow ramp.
     */
    void set_default_grayscale();
    void set_default_fire_palette();

    // ------------------------------------------------------------------------
    // Framebuffer Pixel Operations
    // ------------------------------------------------------------------------

    /**
     * @brief Plot pixel at (x, y) with color index. Ignores out-of-bounds coordinates.
     */
    inline void put_pixel(int x, int y, uint8_t color_index) noexcept {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            m_indexed_buffer[static_cast<size_t>(y * width + x)] = color_index;
        }
    }

    /**
     * @brief Unchecked pixel write (for performance-critical inner loops with verified bounds).
     */
    inline void put_pixel_fast(int x, int y, uint8_t color_index) noexcept {
        m_indexed_buffer[static_cast<size_t>(y * width + x)] = color_index;
    }

    /**
     * @brief Read pixel color index at (x, y). Returns 0 if out of bounds.
     */
    [[nodiscard]] inline uint8_t get_pixel(int x, int y) const noexcept {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            return m_indexed_buffer[static_cast<size_t>(y * width + x)];
        }
        return 0;
    }

    /**
     * @brief Additive pixel stamp for Shaded Bobs and glowing trails:
     * buffer[p] = (buffer[p] + delta) mod (max_val + 1)
     */
    inline void add_pixel(int x, int y, uint8_t delta, uint8_t max_val = 63) noexcept {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            const size_t idx = static_cast<size_t>(y * width + x);
            m_indexed_buffer[idx] = static_cast<uint8_t>((m_indexed_buffer[idx] + delta) & max_val);
        }
    }

    /**
     * @brief Clear entire 320x200 buffer to specified color index (default: 0).
     */
    void clear(uint8_t color_index = 0) noexcept;

    /**
     * @brief Direct pointer access to linear 64,000 byte index buffer ($A000:0000).
     */
    [[nodiscard]] uint8_t* raw_pixels() noexcept { return m_indexed_buffer.data(); }
    [[nodiscard]] const uint8_t* raw_pixels() const noexcept { return m_indexed_buffer.data(); }

    [[nodiscard]] std::span<uint8_t> pixels() noexcept { return std::span<uint8_t>(m_indexed_buffer); }
    [[nodiscard]] std::span<const uint8_t> pixels() const noexcept { return std::span<const uint8_t>(m_indexed_buffer); }

    // ------------------------------------------------------------------------
    // Rendering & Presentation
    // ------------------------------------------------------------------------

    /**
     * @brief Converts the 8-bit indexed buffer via the palette into a 32-bit ARGB texture
     * and copies it to the destination viewport rectangle.
     */
    void render_to_screen(const neutrino::rect& dest_rect);

private:
    void ensure_texture();

    std::array<uint8_t, pixel_count> m_indexed_buffer{};
    std::array<sdlpp::color, 256> m_palette{};
    std::vector<uint32_t> m_rgba_buffer;
    std::optional<sdlpp::texture> m_texture;
};

} // namespace demoscene
