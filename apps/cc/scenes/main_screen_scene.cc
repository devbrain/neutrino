//
// Created by igor on 7/16/24.
//

#include "main_screen_scene.hh"
#include "scenes_registry.hh"
struct any_key {};

main_screen_scene::main_screen_scene(neutrino::texture_id_t upper, neutrino::texture_id_t lower, double rows_per_second,
                                     std::chrono::seconds wait_time)
	: m_upper(upper),
	  m_lower(lower),
	  m_rows_per_second(rows_per_second),
	  m_wait_time(wait_time),
	  m_time_in_state(0),
	  m_hit_bottom(false),
	  m_direction(NONE)
{
}

void main_screen_scene::update(std::chrono::milliseconds delta_time) {
	if (get_event<any_key>()) {
		push_scene(scenes_registry::instance().get(scene_name_t::MAIN_DIALOG));
	} else {
		m_time_in_state += delta_time;
		if (m_direction == NONE) {
			if (std::chrono::duration_cast<std::chrono::seconds>(m_time_in_state) > m_wait_time) {
				m_time_in_state = std::chrono::milliseconds(0);
				if (m_hit_bottom) {
					m_direction = UP;
				} else {
					m_direction = DOWN;
				}
			}
		} else {
			auto rows_ellapsed = static_cast<int>(m_rows_per_second * m_time_in_state.count() / 1000.0);
			if (m_direction == DOWN) {
				m_window.y = rows_ellapsed;
				if (m_window.y >= m_up_dims.h) {
					m_window.y = m_up_dims.h;
					m_direction = NONE;
					m_hit_bottom = true;
					m_time_in_state = std::chrono::milliseconds(0);
				}
			} else {
				m_window.y = m_window.h - rows_ellapsed;
				if (m_window.y <= 0) {
					m_window.y = 0;
					m_direction = NONE;
					m_hit_bottom = false;
					m_time_in_state = std::chrono::milliseconds(0);
				}
			}
		}
	}
}

void main_screen_scene::render(neutrino::sdl::renderer& renderer) {
	neutrino::sdl::rect up_src = m_up_dims;
	up_src.y = m_window.y;
	up_src.h = m_up_dims.h - m_window.y;
	neutrino::sdl::rect down_src = m_down_dims;
	down_src.y = 0;
	down_src.h = m_window.y;

	if (up_src.h >= 1) {
		neutrino::sdl::rect dst;
		dst.x = 0;
		dst.w = m_up_dims.w;
		dst.y = 0;
		dst.h = up_src.h;
		renderer.copy(*m_up, up_src, dst);
	}
	if (down_src.h >= 1) {
		neutrino::sdl::rect dst;
		dst.x = 0;
		dst.w = m_up_dims.w;
		dst.y = up_src.h;
		dst.h = down_src.h;
		renderer.copy(*m_down, down_src, dst);
	}
}



void main_screen_scene::initialize() {
	std::tie(m_up, m_up_dims) = get_texture_atlas().get(m_upper);
	std::tie(m_down, m_down_dims) = get_texture_atlas().get(m_lower);
	m_window = m_up_dims;

	register_event_handler([](const neutrino::sdl::events::keyboard& kb, any_key&) {
		return kb.pressed;
	});
}


