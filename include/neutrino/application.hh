//
// Created by igor on 7/14/24.
//

#ifndef NEUTRINO_APPLICATION_HH
#define NEUTRINO_APPLICATION_HH

#include <stdexcept>
#include <sdlpp/sdlpp.hh>

#include <neutrino/neutrino_export.hh>
#include <neutrino/events/events_reactor.hh>
#include <neutrino/scene/scene_manager.hh>

namespace neutrino {
	class NEUTRINO_EXPORT application {
		public:
			application();
			virtual ~application();

			void init(unsigned w, unsigned h, bool fullscreen = false, int desired_fps = 60);

			virtual void run();

			void quit();
			unsigned get_fps() const;
			static application& instance();

		protected:
			virtual void init_logger();
			virtual void init_vfs();
			virtual void init_assets();
			virtual void setup_scenes(sdl::renderer& renderer) = 0;
			virtual void on_error(const std::exception& exception) const;
			virtual void on_unkown_error() const;

			virtual void on_terminating();
			virtual void on_low_memory();
			virtual void on_before_backgroud();
			virtual void on_in_backgroud();
			virtual void on_before_foreground();
			virtual void on_in_foreground();

		private:
			bool need_to_quit() const;
			bool user_init_sequence();

		private:
			sdl::system m_initializer;
			sdl::window m_main_window;
			sdl::renderer m_renderer;
			scene_manager m_scene_manager;
			events_reactor m_event_reactor;
			bool m_quit_flag;
			int m_desired_fps;
			unsigned m_fps;
	};
}

#endif
