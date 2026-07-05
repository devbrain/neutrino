//
// Created by igor on 02/07/2026.
//

#include "service_locator.hh"

namespace neutrino {
    service_locator& service_locator::instance() {
        static service_locator impl;
        return impl;
    }

    void service_locator::set_window(sdlpp::window& w) {
        m_window = &w;
    }

    void service_locator::set_renderer(sdlpp::renderer& r) {
        m_renderer = &r;
    }

    void service_locator::set_scenes_manager(scenes_manager& s) {
        m_scenes_manager = &s;
    }

    void service_locator::set_application(application& a) {
        m_application = &a;
    }

    void service_locator::set_gamepads(gamepads& g) {
        m_gamepads = &g;
    }

    void service_locator::set_sound_system(sound_system& s) {
        m_sound_system = &s;
    }

    void service_locator::set_texture_registry(texture_registry& s) {
        m_texture_registry = &s;
    }
}
