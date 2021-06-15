//
// Created by igor on 14/06/2021.
//

#include <neutrino/engine/renderer/vga_renderer.hh>
#include <neutrino/sdl/window.hh>
#include <neutrino/sdl/surface.hh>

namespace neutrino::engine {
    struct vga_renderer::impl {
        explicit impl(sdl::window&& w, int width, int height)
        : window(std::move(w)),
          window_surface (sdl::surface(window)),
          work(sdl::surface::make_8bit(width, height))
        {

        }
        sdl::window  window;
        sdl::surface window_surface;
        sdl::surface work;
    };
    // ================================================================================
    vga_renderer::vga_renderer(const basic_window& window) {
        auto w = sdl::window::by_id(window.id());
        auto [width, height] = w.size();
        m_pimpl = std::make_unique<impl>(std::move(w), width, height);
    }
    // --------------------------------------------------------------------------------
    vga_renderer::~vga_renderer() = default;
    // --------------------------------------------------------------------------------
    void vga_renderer::clear() {
        m_pimpl->work.fill(0);
    }
    // --------------------------------------------------------------------------------
    void vga_renderer::present() {
        m_pimpl->work.blit(m_pimpl->window_surface);
        m_pimpl->window.update_surface();
    }
}