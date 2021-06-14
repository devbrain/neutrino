//
// Created by igor on 11/06/2021.
//

#ifndef NEUTRINO_ENGINE_WINDOWS_WINDOW_OPENGL_HH
#define NEUTRINO_ENGINE_WINDOWS_WINDOW_OPENGL_HH

#include <neutrino/engine/windows/engine_window.hh>

namespace neutrino::engine {
    class window_opengl : public engine_window {
    public:
        window_opengl ();
        window_opengl(window_flags_t flags);
    };
}
#endif
