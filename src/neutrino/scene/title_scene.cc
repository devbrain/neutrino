//
// Created by igor on 7/16/24.
//

#include <neutrino/scene/title_scene.hh>
#include <neutrino/application.hh>

namespace neutrino {
	title_scene::title_scene(texture_id_t tid, std::chrono::seconds display_time): m_tid(tid),
		m_time_in_screen(0),
		m_time_to_show(display_time) {

	}

	title_scene::~title_scene() {
		//application::instance().get_texture_atlas().remove()
	}

	void title_scene::update(std::chrono::milliseconds delta_time) {
		m_time_in_screen += delta_time;
		std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(m_time_in_screen);
		bool done = false;
		if (s > m_time_to_show) {
			m_time_in_screen = std::chrono::milliseconds(0);
			done = true;
		}
		if (get_event<key_pressed>()) {
			done = true;
		}
		if (done) {
			when_done();
		}
	}

	void title_scene::render(neutrino::sdl::renderer& renderer) {
		auto [txt_ptr, rect] = get_texture_atlas().get(m_tid);
		renderer.copy(*txt_ptr, rect);
	}

	void title_scene::initialize() {
		register_event_handler([](const sdl::events::keyboard& e, key_pressed&) {
			                       return e.pressed;
		                       }, [](const sdl::events::mouse_button& ev, key_pressed&) {
			                       return ev.pressed;
		                       });
	}
}
