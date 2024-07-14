//
// Created by igor on 7/14/24.
//

#ifndef NEUTRINO_SCENE_HH
#define NEUTRINO_SCENE_HH

#include <chrono>
#include <neutrino/neutrino_export.hh>
#include <neutrino/events/events_reactor.hh>
#include <sdlpp/sdlpp.hh>

namespace neutrino {
	class NEUTRINO_EXPORT scene {
		public:
			scene();
			scene(const scene&) = delete;
			scene& operator =(const scene&) = delete;

			virtual ~scene();

			virtual void handle_input(const sdl::events::event_t& ev);
			virtual void update(std::chrono::milliseconds delta_time) = 0;
			virtual void render(sdl::renderer& renderer) = 0;
		protected:
			template <typename ... Callbacks>
			void register_event_handler(Callbacks ... callbacks) {
				m_events_reactor.register_handler(std::forward<Callbacks>(callbacks)...);
			}
		protected:
			virtual void on_widow_moved(const sdl::point& new_pos);
			virtual void on_widow_resized(const sdl::area_type& new_area);
			virtual void on_window_shown();
			virtual void on_window_hidden();
			virtual void on_window_exposed();
			virtual void on_window_minimized();
			virtual void on_window_maximized();
			virtual void on_window_restored();
			virtual void on_window_mouse_entered();
			virtual void on_window_mouse_leaved();
			virtual void on_window_focus_gained();
			virtual void on_window_focus_lost();
			virtual void on_window_close();
		private:
			events_reactor m_events_reactor;
	};
}

#endif
