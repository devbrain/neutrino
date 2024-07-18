//
// Created by igor on 7/14/24.
//

#include <neutrino/scene/scene.hh>
#include <neutrino/application.hh>
#include "events/neutrino_events.hh"
#include <bsw/logger/logger.hh>
#include <utility>

namespace neutrino {
	scene::scene()
		: m_scene_manager(nullptr),
		  m_is_active(false),
		  m_is_initialized(false) {
	}

	scene::~scene() = default;

	void scene::initialize() {
	}

	void scene::handle_input(const sdl::events::event_t& ev) {
			m_events_reactor.handle(ev);
	}

	scene::flags scene::get_flags() const {
		flags f;
		f.clear();
		return f;
	}

	texture_atlas& scene::get_texture_atlas() {
		return application::instance().get_texture_atlas();
	}

	void scene::push_scene(std::shared_ptr<scene> new_scene) {
		generate_event(push_scene_event{std::move(new_scene)});
	}

	void scene::pop_scene() {
		generate_event(pop_scene_event{});
	}

	void scene::replace_scene(std::shared_ptr<scene> new_scene) {
		generate_event(replace_scene_event{std::move(new_scene)});
	}

	void scene::on_widow_moved(const sdl::point& new_pos) {
	}

	void scene::on_widow_resized(const sdl::area_type& new_area) {
		EVLOG_TRACE(EVLOG_INFO, "Window resized ", new_area);
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
		application::instance().quit();
	}

	void scene::on_timer([[maybe_unused]] void* data) {
	}

	void scene::on_activated() {
	}

	void scene::on_deactived() {
	}

	void scene::on_evicted() {
	}

	bool scene::is_active() const {
		return m_is_active;
	}

	void scene::activate() {
		if (!m_is_initialized) {
			initialize();
			m_is_initialized = true;
		}
		if (!m_is_active) {
			m_is_active = true;
			on_activated();
		}
	}

	void scene::deactivate() {
		if (m_is_active) {
			m_is_active = false;
			on_deactived();
		}
	}

	void scene::evict() {
		deactivate();
		on_evicted();
		m_scene_manager = nullptr;
	}

	bool scene::handle_system_event(const sdl::events::event_t& ev) {
		if (const auto* moved_event = std::get_if <sdl::events::window_moved>(&ev)) {
			on_widow_moved(sdl::point(static_cast <int>(moved_event->x), static_cast <int>(moved_event->y)));
		} else if (const auto* resized_event = std::get_if <sdl::events::window_resized>(&ev)) {
			on_widow_resized(sdl::area_type(static_cast <int>(resized_event->w), static_cast <int>(resized_event->h)));
		} else if (std::get_if <sdl::events::window_shown>(&ev)) {
			on_window_shown();
		} else if (std::get_if <sdl::events::window_hidden>(&ev)) {
			on_window_hidden();
		} else if (std::get_if <sdl::events::window_exposed>(&ev)) {
			on_window_exposed();
		} else if (std::get_if <sdl::events::window_minimized>(&ev)) {
			on_window_minimized();
		} else if (std::get_if <sdl::events::window_maximized>(&ev)) {
			on_window_maximized();
		} else if (std::get_if <sdl::events::window_restored>(&ev)) {
			on_window_restored();
		} else if (std::get_if <sdl::events::window_mouse_entered>(&ev)) {
			on_window_mouse_entered();
		} else if (std::get_if <sdl::events::window_mouse_leaved>(&ev)) {
			on_window_mouse_leaved();
		} else if (std::get_if <sdl::events::window_focus_gained>(&ev)) {
			on_window_focus_gained();
		} else if (std::get_if <sdl::events::window_focus_lost>(&ev)) {
			on_window_focus_lost();
		} else if (std::get_if <sdl::events::window_close>(&ev)) {
			on_window_close();
		} else {
			return false;
		}
		return true;
	}

	void scene::setup_scene_manager(scene_manager* sm) {
		m_scene_manager = sm;
	}
}
