//
// Created by igor on 10/7/24.
//

#ifndef NEUTRINO_INCLUDE_NEUTRINO_MODULES_VIDEO_SYSTEMS_VIDEO_SYSTEM_HH_
#define NEUTRINO_INCLUDE_NEUTRINO_MODULES_VIDEO_SYSTEMS_VIDEO_SYSTEM_HH_

#include <neutrino/modules/video/tile.hh>
#include <neutrino/modules/video/world/animation_sequence.hh>
#include <neutrino/ecs/abstract_system.hh>

namespace neutrino::ecs::video {
 class NEUTRINO_EXPORT video_system : public abstract_system {
	 public:
		video_system();
	 protected:
		[[nodiscard]] const texture_atlas& get_atlas() const;
		[[nodiscard]] const sdl::renderer& get_renderer() const;
	 	[[nodiscard]] sdl::renderer& get_renderer();
		[[nodiscard]] sdl::rect get_screen_dimensions() const;

		void draw_tile(const tile& tid, int screen_x, int screen_y) const;
		void draw_tile(const tile& tid, const sdl::point& screen_point) const;

		void draw_tile(const tile& tid, const sdl::point& world_point, sdl::rect world_viewport) const;
	 	void draw_tile(const tile& tid, int world_x, int world_y, sdl::rect world_viewport) const;

		void draw_tile(const tiled::animation_sequence& seq, unsigned frame, int screen_x, int screen_y) const;
		void draw_tile(const tiled::animation_sequence& seq, unsigned frame, const sdl::point& screen_point) const;
	 	void draw_tile(const tiled::animation_sequence& seq, unsigned frame, const sdl::point& screen_point, sdl::rect world_viewport) const;
	 	void draw_tile(const tiled::animation_sequence& seq, unsigned frame, int world_x, int world_y, sdl::rect world_viewport) const;
	 private:
		texture_atlas& m_atlas;
		sdl::renderer& m_renderer;
		sdl::rect      m_window_dimensions;
	};
}

#endif
