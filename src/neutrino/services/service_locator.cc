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

    void service_locator::set_sprites_manager(sprites_manager& s) {
        m_sprites_manager = &s;
    }

    void service_locator::clear_application(application& a) noexcept {
        if (m_application != &a) {
            return;
        }

        m_renderer = nullptr;
        m_window = nullptr;
        m_scenes_manager = nullptr;
        m_application = nullptr;
        m_gamepads = nullptr;
        m_sound_system = nullptr;
        m_texture_registry = nullptr;
        m_sprites_manager = nullptr;
    }
}
