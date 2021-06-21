//
// Created by igor on 21/06/2021.
//

#include <neutrino/engine/application/oldschool_demo.hh>

namespace neutrino::engine {
    oldschool_demo::oldschool_demo ()
    : m_video_system(std::make_shared<systems::video::oldschool>(320, 200)) {

    }

    oldschool_demo::oldschool_demo (int w, int h)
            : m_video_system(std::make_shared<systems::video::oldschool>(w, h)) {

    }

    std::shared_ptr<systems::video::oldschool> oldschool_demo::video_system() const noexcept {
        return m_video_system;
    }
}