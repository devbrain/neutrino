//
// Created by igor on 7/14/24.
//

#ifndef NEUTRINO_SCENE_HH
#define NEUTRINO_SCENE_HH

#include <chrono>
#include <neutrino/neutrino_export.hh>
#include <neutrino/events/events_reactor.hh>
#include <neutrino/systems/video/texture_atlas.hh>
#include <sdlpp/sdlpp.hh>
#include <bitflags/bitflags.hpp>

namespace neutrino {
	class NEUTRINO_EXPORT scene_manager;

	class NEUTRINO_EXPORT scene {
		friend class scene_manager;

		public:
			BEGIN_BITFLAGS(flags)
				FLAG(NONE)
				FLAG(TRANSPARENT)
				FLAG(PROPAGATE_EVENTS)
				FLAG(PROPAGATE_UPDATES)
			END_BITFLAGS(flags)

		public:
			scene();
			scene(const scene&) = delete;
			scene& operator =(const scene&) = delete;

			virtual ~scene();

		protected:
			virtual void initialize();
			virtual void handle_input(const sdl::events::event_t& ev);
			virtual void update(std::chrono::milliseconds delta_time) = 0;
			virtual void render(sdl::renderer& renderer) = 0;
			virtual flags get_flags() const;

			template<typename... Callbacks>
			void register_event_handler(Callbacks... callbacks) {
				m_events_reactor.register_handler(std::forward <Callbacks>(callbacks)...);
			}

			template <typename Event>
			const Event* get_event() {
				return m_events_reactor.get<Event>();
			}

			virtual texture_atlas& get_texture_atlas();

			static void push_scene(std::shared_ptr<scene> new_scene);
			static void pop_scene();
			static void replace_scene(std::shared_ptr<scene> new_scene);
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

			virtual void on_timer(void* data);
		protected:
			virtual void on_activated();
			virtual void on_deactived();
			virtual void on_evicted();
			bool is_active() const;
		private:
			// this method is called when this scene becomes active (top) scene
			void activate();
			// this method is called when this scene becomes not active
			void deactivate();
			// this method is called when this scene is poped from the scene manager
			void evict();
			bool handle_system_event(const sdl::events::event_t& ev);
		private:
			void setup_scene_manager(scene_manager* sm);
		private:
			events_reactor m_events_reactor;
			scene_manager* m_scene_manager;
			bool m_is_active;
			bool m_is_initialized;
	};
}

#endif
