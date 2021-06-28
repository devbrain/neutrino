//
// Created by igor on 21/06/2021.
//

#include <neutrino/engine/application/oldschool_demo.hh>

namespace neutrino::engine {
    oldschool_demo::oldschool_demo ()
    : m_video_system(std::make_shared<systems::video::oldschool>(320, 200)),
      m_colors(256) {

    }
    // ----------------------------------------------------------------------------------
    oldschool_demo::oldschool_demo (int w, int h)
    : m_video_system(std::make_shared<systems::video::oldschool>(w, h)),
      m_colors(256) {

    }
    // -----------------------------------------------------------------------------------
    std::shared_ptr<systems::video::oldschool> oldschool_demo::video_system() const noexcept {
        return m_video_system;
    }
    // -----------------------------------------------------------------------------------
    void oldschool_demo::init_palette([[maybe_unused]] sdl::palette& colors) {

    }
    // -----------------------------------------------------------------------------------
    void oldschool_demo::pre_run() {

    }
    // -----------------------------------------------------------------------------------
    void oldschool_demo::after_window_opened() {
      //  m_colors = vga_renderer::standard_palette();
        init_palette(m_colors);
        video_system()->palette(m_colors);
        pre_run();
    }
    // -----------------------------------------------------------------------------------
    void oldschool_demo::update(std::chrono::milliseconds ms) {
        update(m_video_system->pixels(), ms);
    }
}