//
// Created by igor on 03/07/2021.
//

#ifndef NEUTRINO_ENGINE_SCENE_MANAGER_HH
#define NEUTRINO_ENGINE_SCENE_MANAGER_HH

#include <neutrino/engine/events.hh>
#include <neutrino/utils/singleton.hh>

namespace neutrino::engine {

    class window;
    namespace detail
    {
        class scene_manager
        {
            friend class neutrino::engine::window;
        public:

        private:
            void on_before_destroy();
            void on_input_focus_changed(bool keyboard_focus, bool mouse_focus);
            void on_visibility_change(bool is_visible);
            void on_keyboard_input(const events::keyboard& ev);
            void on_pointer_input(const events::pointer& ev);
        };
    } // ns detail

    using scene_manager = utils::singleton<detail::scene_manager>;
}

#endif
