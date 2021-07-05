//
// Created by igor on 03/07/2021.
//

#include "video.hh"
#include <neutrino/hal/video/pixel_format.hh>

namespace neutrino::demoscene {
    video::video (int w, int h)
    : m_width(w), m_heigh(h), m_format(hal::pixel_format::make_rgba_32bit()) {
        m_surface.resize(w*h, 0);
        m_palette.fill({0,0,0,0});
    }

    void video::attach(const hal::window& window) {
        engine::accelerated_2d_context::attach(window);
        m_renderer.logical_size(m_width, m_heigh);
        m_texture = hal::texture(m_renderer, m_format, m_width, m_heigh, hal::texture::access::STREAMING);
    }

    void video::present() {
        auto [px, pitch] = m_texture.lock();
        auto* pixels = (uint32_t*)px;
        std::size_t idx = 0;
        for (int y=0; y<m_heigh; y++) {
            for (int x=0; x<m_width; x++) {
                pixels[idx] = m_texture.map_rgb(m_palette[m_surface[idx]]);
                idx++;
            }
        }
        m_texture.unlock();
        m_renderer.copy(m_texture);
        engine::accelerated_2d_context::present();
    }

    std::tuple<int, int> video::dimensions() {
        return {m_width, m_heigh};
    }

    palette_t* video::palette () {
        return &m_palette;
    }

    surface_t* video::surface() {
        return &m_surface;
    }
}