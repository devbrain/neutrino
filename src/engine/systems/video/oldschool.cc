//
// Created by igor on 21/06/2021.
//

#include <neutrino/engine/systems/video/oldschool.hh>

namespace neutrino::engine::systems::video {
    oldschool::oldschool(int width, int height)
    : video_system<retro::renderer>(width, height, false)
    {

    }
}