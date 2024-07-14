//
// Created by igor on 7/14/24.
//

#include <neutrino/scene/scene.hh>

namespace neutrino {
	scene::scene() = default;
	scene::~scene() = default;

	void scene::handle_input(const sdl::events::event_t& ev) {
		if (const auto* moved_event = std::get_if<sdl::events::window_moved>(&ev)) {
			on_widow_moved(sdl::point(static_cast<int>(moved_event->x), static_cast <int>(moved_event->y)));
		} else if (const auto* resized_event = std::get_if<sdl::events::window_resized>(&ev)) {
			on_widow_resized(sdl::area_type(static_cast <int>(resized_event->w), static_cast <int>(resized_event->h)));
		} else if (std::get_if<sdl::events::window_shown>(&ev)) {
			on_window_shown();
		} else if (std::get_if<sdl::events::window_hidden>(&ev)) {
			on_window_hidden();
		} else if (std::get_if<sdl::events::window_exposed>(&ev)) {
			on_window_exposed();
		} else if (std::get_if<sdl::events::window_minimized>(&ev)) {
			on_window_minimized();
		} else if (std::get_if<sdl::events::window_maximized>(&ev)) {
			on_window_maximized();
		} else if (std::get_if<sdl::events::window_restored>(&ev)) {
			on_window_restored();
		} else if (std::get_if<sdl::events::window_mouse_entered>(&ev)) {
			on_window_mouse_entered();
		} else if (std::get_if<sdl::events::window_mouse_leaved>(&ev)) {
			on_window_mouse_leaved();
		} else if (std::get_if<sdl::events::window_focus_gained>(&ev)) {
			on_window_focus_gained();
		} else if (std::get_if<sdl::events::window_focus_lost>(&ev)) {
			on_window_focus_lost();
		} else if (std::get_if<sdl::events::window_close>(&ev)) {
			on_window_close();
		} else {
			m_events_reactor.handle(ev);
		}
	}

	void scene::on_widow_moved(const sdl::point& new_pos) {
	}

	void scene::on_widow_resized(const sdl::area_type& new_area) {
	}

	void scene::on_window_shown() {
	}

	void scene::on_window_hidden() {
	}

	void scene::on_window_exposed() {
	}

	void scene::on_window_minimized() {
	}

	void scene::on_window_maximized() {
	}

	void scene::on_window_restored() {
	}

	void scene::on_window_mouse_entered() {
	}

	void scene::on_window_mouse_leaved() {
	}

	void scene::on_window_focus_gained() {
	}

	void scene::on_window_focus_lost() {
	}

	void scene::on_window_close() {
	}
}
