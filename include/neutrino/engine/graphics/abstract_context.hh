//
// Created by igor on 02/07/2021.
//

#ifndef NEUTRINO_GRAPHICS_ABSTRACT_CONTEXT_HH
#define NEUTRINO_GRAPHICS_ABSTRACT_CONTEXT_HH

#include <tuple>
#include <neutrino/hal/video/window.hh>

namespace neutrino::engine {
    class abstract_graphics_context {
    public:
        virtual ~abstract_graphics_context();

        virtual void open(const hal::window& window) = 0;
        virtual void clear() = 0;
        virtual void present() = 0;

        [[nodiscard]] virtual hal::window::window_kind_t window_kind() const noexcept = 0;
        virtual void invalidate(const hal::window& window) = 0;

        // logical dimensions (w,h) of the gc
        virtual std::tuple<int, int> dimensions() = 0;
    };
}

#endif

