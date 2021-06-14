//
// Created by igor on 12/06/2021.
//

#include <neutrino/engine/windows/window_opengl.hh>

namespace neutrino::engine {
    window_opengl::window_opengl() {}

    window_opengl::window_opengl(window_flags_t flags)
            : engine_window(basic_window::OPENGL, flags)
    {

    }
}