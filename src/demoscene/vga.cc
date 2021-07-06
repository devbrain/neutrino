//
// Created by igor on 03/07/2021.
//

#include <neutrino/demoscene/vga.hh>
#include <neutrino/hal/video/pixel_format.hh>

namespace neutrino::demoscene {
    vga::vga (engine::main_window& window)
    :   m_window(window),
        m_renderer(window.get_renderer())
        {

        auto [w,h] = window.dimensions();
        m_surface.resize(w*h, 0);
        m_palette.fill({0,0,0,0});
        m_renderer.logical_size(w, h);
        m_texture = hal::texture(m_renderer, hal::pixel_format::make_rgba_32bit(), w, h, hal::texture::access::STREAMING);
    }

    void vga::present() {
        auto [w,h] = m_renderer.logical_size();
        auto [px, pitch] = m_texture.lock();
        auto* pixels = (uint32_t*)px;
        std::size_t idx = 0;
        for (int y=0; y<h; y++) {
            for (int x=0; x<w; x++) {
                pixels[idx] = m_texture.map_rgb(m_palette[m_surface[idx]]);
                idx++;
            }
        }
        m_texture.unlock();
        m_renderer.copy(m_texture);
    }

    int vga::width () const noexcept {
        int w;
        std::tie(w, std::ignore) = m_window.dimensions();
        return w;
    }

    int vga::height () const noexcept {
        int h;
        std::tie(std::ignore, h) = m_window.dimensions();
        return h;
    }

    vga::palette_t& vga::palette () {
        return m_palette;
    }

    vga::surface_t& vga::surface() {
        return m_surface;
    }
}