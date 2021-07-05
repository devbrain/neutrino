//
// Created by igor on 03/07/2021.
//

#ifndef NEUTRINO_ENGINE_SCENE_MANAGER_HH
#define NEUTRINO_ENGINE_SCENE_MANAGER_HH

#include <map>
#include <memory>
#include <chrono>

#include <neutrino/engine/events.hh>
#include <neutrino/utils/singleton.hh>
#include <neutrino/engine/scene.hh>
#include <neutrino/utils/observer.hh>
#include <neutrino/engine/events.hh>


namespace neutrino::engine {

    class window;
    class application;
    namespace detail
    {
        class scene_manager : public utils::publisher<events::full_screen, events::quit>
        {
            friend class neutrino::engine::window;
            friend class neutrino::engine::application;

        public:
            scene_manager();

            void add(scene* sc);
            void activate(int scene_id);
        private:
            void update(std::chrono::milliseconds ms);
            void on_before_destroy();
            void on_input_focus_changed(bool keyboard_focus, bool mouse_focus);
            void on_visibility_change(bool is_visible);
            void on_keyboard_input(const events::keyboard& ev);
            void on_pointer_input(const events::pointer& ev);
        private:
            std::map<int, scene*> m_scenes;
            scene* m_current_scene;
            bool   m_first_time;
        };
    } // ns detail

    using scene_manager = utils::singleton<detail::scene_manager>;
}

#endif
