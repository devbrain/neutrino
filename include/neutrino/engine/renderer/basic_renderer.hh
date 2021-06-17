//
// Created by igor on 14/06/2021.
//

#ifndef NEUTRINO_RENDERER_BASIC_RENDERER_HH
#define NEUTRINO_RENDERER_BASIC_RENDERER_HH

#include <neutrino/engine/windows/basic_window.hh>

namespace neutrino::engine {
    class basic_renderer {
    public:
        virtual ~basic_renderer();
        virtual void open(const basic_window& window) = 0;
        virtual void clear() = 0;
        virtual void present() = 0;

        virtual basic_window::window_kind_t window_kind() const noexcept = 0;
    };
}

#endif
