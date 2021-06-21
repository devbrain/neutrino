//
// Created by igor on 21/06/2021.
//

#ifndef NEUTRINO_SYSTEMS_VIDEO_OLDSCHOOL_HH
#define NEUTRINO_SYSTEMS_VIDEO_OLDSCHOOL_HH

#include <neutrino/engine/systems/video/video_system.hh>
#include <neutrino/engine/systems/video/retro/retro.hh>

namespace neutrino::engine::systems::video {
    class oldschool : public video_system<retro::renderer> {
    public:
        oldschool(int width, int height);
    };
}

#endif
