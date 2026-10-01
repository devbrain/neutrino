#include "vga_canvas.hh"

#include <algorithm>
#include <cstring>
#include <sdlpp/video/renderer.hh>

namespace demoscene {

vga_canvas::vga_canvas()
    : m_rgba_buffer(pixel_count, 0xFF000000)
{
    set_default_grayscale();
}

void vga_canvas::set_rgb_6bit(uint8_t index, uint8_t r6, uint8_t g6, uint8_t b6) {
    // 6-bit DAC (0..63) to 8-bit (0..255) expansion: (val * 255) / 63
    const uint8_t r8 = static_cast<uint8_t>((static_cast<uint32_t>(r6 & 0x3F) * 255) / 63);
    const uint8_t g8 = static_cast<uint8_t>((static_cast<uint32_t>(g6 & 0x3F) * 255) / 63);
    const uint8_t b8 = static_cast<uint8_t>((static_cast<uint32_t>(b6 & 0x3F) * 255) / 63);
    m_palette[index] = sdlpp::color{r8, g8, b8, 255};
}

void vga_canvas::set_rgb(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    m_palette[index] = sdlpp::color{r, g, b, a};
}

void vga_canvas::set_color(uint8_t index, const sdlpp::color& color) {
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
        m_palette[static_cast<size_t>(i)] = sdlpp::color{v, v, v, 255};
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
        auto tex_exp = sdlpp::texture::create(
            neutrino::get_renderer(),
            sdlpp::pixel_format_enum::RGBA8888,
            sdlpp::texture_access::streaming,
            width, height
        );
        if (tex_exp) {
            m_texture = std::move(*tex_exp);
            (void)m_texture->set_scale_mode(sdlpp::scale_mode::nearest);
        }
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

    // 2. Translate 8-bit indexed buffer to 32-bit RGBA streaming buffer
    for (size_t i = 0; i < pixel_count; ++i) {
        m_rgba_buffer[i] = lut[m_indexed_buffer[i]];
    }

    // 3. Update streaming texture and blit to destination rectangle
    (void)m_texture->update(m_rgba_buffer.data(), static_cast<int>(width * sizeof(uint32_t)));
    (void)neutrino::get_renderer().copy(*m_texture, std::nullopt, dest_rect);
}

} // namespace demoscene
