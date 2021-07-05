//
// Created by igor on 02/07/2021.
//

#include <neutrino/engine/graphics/accelerated_2d_context.hh>
namespace neutrino::engine
{
    accelerated_2d_context::accelerated_2d_context () = default;

    void accelerated_2d_context::attach(const hal::window& window) {
        m_renderer = hal::renderer(window, hal::renderer::flags::ACCELERATED);
    }

    void accelerated_2d_context::clear() {
        m_renderer.clear();
    }

    void accelerated_2d_context::present() {
        m_renderer.present();
    }

    hal::window::window_kind_t accelerated_2d_context::window_kind() const noexcept {
        return hal::window::SIMPLE;
    }

    void accelerated_2d_context::invalidate([[maybe_unused]] const hal::window& window) {

    }

}