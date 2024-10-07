//
// Created by igor on 10/7/24.
//

#include <neutrino/application.hh>
#include <neutrino/modules/video/systems/video_system.hh>


namespace neutrino::ecs::video {

	video_system::video_system()
		: m_atlas(application::instance().get_texture_atlas()),
		  m_renderer(application::instance().get_renderer()),
		  m_window_dimensions(application::instance().get_window_dimensions()) {
	}

	const texture_atlas& video_system::get_atlas() const {
		return m_atlas;
	}

	const sdl::renderer& video_system::get_renderer() const {
		return m_renderer;
	}

	sdl::renderer& video_system::get_renderer() {
		return m_renderer;
	}

	sdl::rect video_system::get_screen_dimensions() const {
		return m_window_dimensions;
	}

	void video_system::draw_tile(const tile& tid, int screen_x, int screen_y) const {
		auto [text_ptr, rect] = get_atlas().get(tid);
		if (text_ptr) {
			const sdl::rect dst_rect(screen_x, screen_y, rect.w, rect.h);
			m_renderer.copy(*text_ptr, rect, dst_rect);
		}
	}

	void video_system::draw_tile(const tile& tid, const sdl::point& screen_point) const {
		draw_tile(tid, screen_point.x, screen_point.y);
	}

	void video_system::draw_tile(const tiled::animation_sequence& seq, unsigned frame, int screen_x, int screen_y) const {
		ENFORCE(frame < seq.get_frames().size());
		draw_tile(seq.get_frames()[frame].m_tile, screen_x, screen_y);
	}

	void video_system::draw_tile(const tiled::animation_sequence& seq, unsigned frame,
										   const sdl::point& screen_point) const {
		draw_tile(seq, frame, screen_point.x, screen_point.y);
	}

	void video_system::draw_tile(const tile& tid, const sdl::point& world_point, sdl::rect world_viewport) const {
		auto [text_ptr, rect] = get_atlas().get(tid);
		if (text_ptr) {
			const sdl::rect dst_rect(world_point.x, world_point.y, rect.w, rect.h);
			auto irect = world_viewport.intersection(dst_rect);
			if (irect) {
				rect.x = irect->x - world_point.x;
				rect.y = irect->y - world_point.y;
				rect.w = irect->w;
				rect.h = irect->h;
				m_renderer.copy(*text_ptr, rect, *irect);
			}

		}
	}

	void video_system::draw_tile(const tile& tid, int screen_x, int screen_y, sdl::rect world_viewport) const {
		draw_tile(tid, sdl::point (screen_x, screen_y), world_viewport);
	}

	void
	video_system::draw_tile(const tiled::animation_sequence& seq, unsigned int frame, const sdl::point& screen_point, sdl::rect world_viewport) const {
		ENFORCE(frame < seq.get_frames().size());
		draw_tile(seq.get_frames()[frame].m_tile, screen_point, world_viewport);
	}

	void
	video_system::draw_tile(const tiled::animation_sequence& seq, unsigned int frame, int world_x, int world_y, sdl::rect world_viewport) const {
		draw_tile(seq, frame, sdl::point (world_x, world_y), world_viewport);
	}
}