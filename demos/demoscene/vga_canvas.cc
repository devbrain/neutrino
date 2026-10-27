#include "vga_canvas.hh"

#include <algorithm>
#include <cstring>

namespace demoscene {

vga_canvas::vga_canvas() {
    set_default_grayscale();
}

void vga_canvas::set_rgb_6bit(uint8_t index, uint8_t r6, uint8_t g6, uint8_t b6) {
    // 6-bit DAC (0..63) to 8-bit (0..255) expansion: (val * 255) / 63
    const uint8_t r8 = static_cast<uint8_t>((static_cast<uint32_t>(r6 & 0x3F) * 255) / 63);
    const uint8_t g8 = static_cast<uint8_t>((static_cast<uint32_t>(g6 & 0x3F) * 255) / 63);
    const uint8_t b8 = static_cast<uint8_t>((static_cast<uint32_t>(b6 & 0x3F) * 255) / 63);
    m_palette[index] = neutrino::color{r8, g8, b8, 255};
}

void vga_canvas::set_rgb(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    m_palette[index] = neutrino::color{r, g, b, a};
}

void vga_canvas::set_color(uint8_t index, const neutrino::color& color) {
    m_palette[index] = color;
}

void vga_canvas::cycle_palette(uint8_t start_index, uint8_t count, int step) {
    if (count <= 1) return;
    const int effective_step = (step % count + count) % count;
    if (effective_step == 0) return;

    std::rotate(
        m_palette.begin() + start_index,
        m_palette.begin() + start_index + effective_step,
        m_palette.begin() + start_index + count
    );
}

void vga_canvas::set_default_grayscale() {
    for (int i = 0; i < 256; ++i) {
        const uint8_t v = static_cast<uint8_t>(i);
        m_palette[static_cast<size_t>(i)] = neutrino::color{v, v, v, 255};
    }
}

void vga_canvas::set_default_fire_palette() {
    for (int i = 0; i < 64; ++i) {
        set_rgb_6bit(static_cast<uint8_t>(i), static_cast<uint8_t>(i), 0, 0); // Black to Red
    }
    for (int i = 0; i < 64; ++i) {
        set_rgb_6bit(static_cast<uint8_t>(64 + i), 63, static_cast<uint8_t>(i), 0); // Red to Yellow
    }
    for (int i = 0; i < 64; ++i) {
        set_rgb_6bit(static_cast<uint8_t>(128 + i), 63, 63, static_cast<uint8_t>(i)); // Yellow to White
    }
    for (int i = 0; i < 64; ++i) {
        set_rgb_6bit(static_cast<uint8_t>(192 + i), 63, 63, 63); // Saturated White
    }
}

void vga_canvas::clear(uint8_t color_index) noexcept {
    std::fill(m_indexed_buffer.begin(), m_indexed_buffer.end(), color_index);
}

void vga_canvas::ensure_texture() {
    if (!m_texture) {
        // Streaming, RGBA8888, nearest-filtered (the neutrino::texture defaults).
        m_texture = neutrino::texture::create(neutrino::dim{width, height}, neutrino::texture_access::streaming);
    }
}

void vga_canvas::render_to_screen(const neutrino::rect& dest_rect) {
    ensure_texture();
    if (!m_texture) return;

    // 1. Precompute 32-bit RGBA palette lookup table for maximum throughput
    uint32_t lut[256];
    for (size_t i = 0; i < 256; ++i) {
        const auto& c = m_palette[i];
        lut[i] = (static_cast<uint32_t>(c.r) << 24)
               | (static_cast<uint32_t>(c.g) << 16)
               | (static_cast<uint32_t>(c.b) << 8)
               | static_cast<uint32_t>(c.a);
    }

    // 2. Translate the 8-bit indexed buffer straight into the locked texture rows
    //    (uploaded when the lock goes out of scope)
    {
        auto lock = m_texture->lock();
        for (int y = 0; y < height; ++y) {
            const uint8_t* src = m_indexed_buffer.data() + static_cast<size_t>(y) * width;
            auto row = lock.row(y);
            for (int x = 0; x < width; ++x) {
                row[static_cast<size_t>(x)] = lut[src[x]];
            }
        }
    }

    // 3. Draw the texture into the destination rectangle
    neutrino::draw_texture(*m_texture, dest_rect);
}

} // namespace demoscene
