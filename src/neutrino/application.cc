//
// Created by igor on 7/13/24.
//

#include <neutrino/application.hh>
#include <bsw/exception.hh>
#include <bsw/logger/logger.hh>

#include "bsw/logger/system.hh"

namespace neutrino {
	static application* s_instance = nullptr;

	application::application()
		: m_quit_flag(false),
		  m_desired_fps(60),
		  m_fps(0) {
	}

	application::~application() {
		bsw::logger::shutdown();
	}

	void application::init(unsigned w, unsigned h, bool fullscreen, int desired_fps) {
		m_desired_fps = desired_fps;
		m_main_window = fullscreen
			                ? sdl::window(static_cast <int>(w), static_cast <int>(h),
			                              sdl::window::flags::FULL_SCREEN_DESKTOP)
			                : sdl::window(static_cast <int>(w), static_cast <int>(h));
		m_renderer = sdl::renderer(m_main_window);
		m_quit_flag = !user_init_sequence();
		if (!m_quit_flag) {
			s_instance = this;
		}
	}

	void application::run() {
		static uint64_t frames = 0;
		static std::chrono::milliseconds total_time(0);

		unsigned old_fps = 0;

		const auto screen_ms_per_frame = std::chrono::milliseconds(1 + (1000 / m_desired_fps));
		auto start = sdl::get_ms_since_init();
		while (!need_to_quit()) {
			const auto s = sdl::get_ms_since_init();
			const auto delta_t = s - start;
			start = s;

			if (!internal_run(delta_t)) {
				return;
			}

			const auto e = sdl::get_ms_since_init();
			const auto ms = e - s;
			frames++;
			if (need_to_quit()) {
				if (screen_ms_per_frame > ms) {
					const auto delta = screen_ms_per_frame - ms;
					sdl::delay(delta);
					total_time += delta;
				} else {
					total_time += ms;
				}
#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
				m_fps = (1000u * frames) / static_cast <unsigned long>(total_time.count());
#if defined(_MSC_VER)
#pragma warning (pop)
#endif
				if (old_fps == 0) {
					old_fps = m_fps;
				}
				if (frames > 1000u) {
					frames = 0u;
					total_time = std::chrono::milliseconds(0);
				}
			}
		}
	}

	void application::quit() {
		m_quit_flag = true;
	}

	unsigned application::get_fps() const {
		return m_fps;
	}

	application& application::instance() {
		ENFORCE(s_instance);
		return *s_instance;
	}

	void application::init_logger() {
	}

	void application::init_vfs() {
	}

	void application::init_assets() {
	}

	void application::on_error(const std::exception& exception) const {
		EVLOG_TRACE(EVLOG_FATAL, exception.what());
	}

	void application::on_unkown_error() const {
		EVLOG_TRACE(EVLOG_FATAL, "Unknown error");
	}

	void application::on_terminating() {
	}

	void application::on_low_memory() {
	}

	void application::on_before_backgroud() {
	}

	void application::on_in_backgroud() {
	}

	void application::on_before_foreground() {
	}

	void application::on_in_foreground() {
	}

	bool application::need_to_quit() const {
		return m_quit_flag || !s_instance || m_scene_manager.empty();
	}

	bool application::user_init_sequence() {
		try {
			init_logger();
			init_vfs();
			init_assets();
			setup_scenes(m_renderer);
		} catch (const std::exception& e) {
			on_error(e);
			return false;
		} catch (...) {
			on_unkown_error();
			return false;
		}
		return true;
	}

	bool application::internal_run(std::chrono::milliseconds delta_t) {
		try {
			const auto top_scene = m_scene_manager.top();
			ENFORCE(top_scene);

			SDL_Event sdl_event;
			while (SDL_PollEvent(&sdl_event)) {
				bool do_quit = false;
				bool system_event_handeled = true;
				switch (static_cast <unsigned int>(sdl_event.window.event)) {
					case SDL_QUIT:
						do_quit = true;
						break;
					case SDL_APP_TERMINATING:
						on_terminating();
						do_quit = true;
						break;
					case SDL_APP_LOWMEMORY:
						on_low_memory();
						break;
					case SDL_APP_WILLENTERBACKGROUND:
						on_before_backgroud();
						break;
					case SDL_APP_DIDENTERBACKGROUND:
						on_in_backgroud();
						break;
					case SDL_APP_WILLENTERFOREGROUND:
						on_before_foreground();
						break;
					case SDL_APP_DIDENTERFOREGROUND:
						on_in_foreground();
						break;
					default:
						system_event_handeled = false;
						break;
				}
				if (do_quit) {
					m_quit_flag = true;
					break;
				}
				if (!system_event_handeled) {
					auto internal_event = sdl::map_event(sdl_event);
					m_event_reactor.handle(internal_event);
					top_scene->handle_input(internal_event);
				}
			}
			top_scene->update(delta_t);
			top_scene->render(m_renderer);
		} catch (const std::exception& e) {
			on_error(e);
			return false;
		} catch (...) {
			on_unkown_error();
			return false;
		}
		return true;
	}
}
