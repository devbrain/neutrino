//
// Created by igor on 19/07/2021.
//

#ifndef NEUTRINO_TILED_WINDOW_HH
#define NEUTRINO_TILED_WINDOW_HH

#include <neutrino/engine/main_window.hh>

namespace neutrino::tiled {
    class window : public engine::main_window {
    public:
        window(int w, int h);
        ~window();
    protected:
        void after_window_opened() override;
    };
}

#endif
