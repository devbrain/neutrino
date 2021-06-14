//
// Created by igor on 12/06/2021.
//

#include <neutrino/engine/windows/window_2d.hh>
#include <neutrino/sdl/window.hh>
#include <neutrino/sdl/render.hh>

namespace neutrino::engine {

    struct window_2d::impl {
        sdl::renderer renderer;
    };
    // -------------------------------------------------------------------------------------------------
    window_2d::window_2d() : m_pimpl(new impl) {

    }
    // -------------------------------------------------------------------------------------------------
    window_2d::window_2d(window_flags_t flags) : engine_window(flags), m_pimpl(new impl) {

    }
    // -------------------------------------------------------------------------------------------------
    window_2d::~window_2d() = default;
    // -------------------------------------------------------------------------------------------------
    void window_2d::clear() {
        m_pimpl->renderer.active_color({0, 0, 0, 0xFF});
        m_pimpl->renderer.clear();
    }
    // -------------------------------------------------------------------------------------------------
    void window_2d::after_window_opened(uint32_t window_id) {
        auto window = sdl::window::by_id(window_id);
        m_pimpl->renderer = sdl::renderer(window, sdl::renderer::flags::ACCELERATED);
    }
    // -------------------------------------------------------------------------------------------------
    void window_2d::before_window_destroy() {
        std::unique_ptr<impl> empty;
        std::swap(m_pimpl, empty);
    }
    // -------------------------------------------------------------------------------------------------
    void window_2d::present() {
        m_pimpl->renderer.present();
    }
    // -------------------------------------------------------------------------------------------------
    void window_2d::render() {

    }
}