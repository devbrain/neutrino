//
// Created by igor on 14/06/2021.
//

#include <neutrino/engine/renderer/renderer_2d.hh>
#include <neutrino/sdl/window.hh>
#include <neutrino/sdl/render.hh>

namespace neutrino::engine {
    struct renderer_2d::impl {
        explicit impl(sdl::window&& w)
        : renderer(w, sdl::renderer::flags::ACCELERATED)
        {

        }
        sdl::renderer renderer;
    };
    // ================================================================================
    void renderer_2d::open (const basic_window& window) {
        auto w = sdl::window::by_id(window.id());
        m_pimpl = std::make_unique<impl>(std::move(w));
    }
    // --------------------------------------------------------------------------------
    renderer_2d::~renderer_2d() = default;
    // --------------------------------------------------------------------------------
    void renderer_2d::clear() {
        m_pimpl->renderer.active_color({0, 0, 0, 0xFF});
        m_pimpl->renderer.clear();
    }
    // --------------------------------------------------------------------------------
    void renderer_2d::present() {
        m_pimpl->renderer.present();
    }
    // --------------------------------------------------------------------------------
    basic_window::window_kind_t renderer_2d::window_kind() const noexcept
    {
        return basic_window::SIMPLE;
    }
}