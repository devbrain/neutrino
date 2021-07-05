//
// Created by igor on 02/07/2021.
//

#ifndef NEUTRINO_GRAPHICS_ACCELERATED_2D_CONTEXT_HH
#define NEUTRINO_GRAPHICS_ACCELERATED_2D_CONTEXT_HH

#include <neutrino/engine/graphics/abstract_context.hh>
#include <neutrino/hal/video/renderer.hh>

namespace neutrino::engine {
    class accelerated_2d_context : public abstract_graphics_context {
    public:
        accelerated_2d_context();

    protected:
        void attach(const hal::window& window) override;
        void clear() override;
        void present() override;

        [[nodiscard]] hal::window::window_kind_t window_kind() const noexcept override;
        void invalidate(const hal::window& window) override;
    protected:
        hal::renderer m_renderer;
    };
}

#endif

