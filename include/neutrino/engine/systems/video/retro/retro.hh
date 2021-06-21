//
// Created by igor on 21/06/2021.
//

#ifndef NEUTRINO_SYSTEMS_VIDEO_RETRO_RETRO_HH
#define NEUTRINO_SYSTEMS_VIDEO_RETRO_RETRO_HH

#include <neutrino/engine/renderer/vga_renderer.hh>

namespace neutrino::engine::systems::video::retro {
    class renderer : public neutrino::engine::vga_renderer {
    public:
        renderer();
        ~renderer();
    };
}

#endif
