//
// Created by igor on 7/4/24.
//

#ifndef  NEUTRINO_MODULES_VIDEO_WORLD_RENDERER_HH
#define  NEUTRINO_MODULES_VIDEO_WORLD_RENDERER_HH

#include <set>
#include <sdlpp/video/render.hh>
#include <sdlpp/video/geometry.hh>
#include <neutrino/modules/video/world/world_model.hh>
#include <neutrino/modules/video/texture_atlas.hh>
#include <neutrino/neutrino_export.hh>

namespace neutrino {
	class NEUTRINO_EXPORT world_renderer {
		public:
			world_renderer(sdl::renderer& renderer,
			               const sdl::area_type& dimensions,
			               sdl::pixel_format pixel_format = sdl::pixel_format::RGBA8888);

			world_renderer(sdl::renderer& renderer,
					   const sdl::area_type& dimensions,
					   const sdl::point& dest_point,
					   sdl::pixel_format pixel_format = sdl::pixel_format::RGBA8888);
			~world_renderer();

			void init(texture_atlas& atlas, const tiled::world_model& model);
			void deinit();

			void update(std::chrono::milliseconds delta_time);

			void present() const;
			[[nodiscard]] sdl::point set_camera (const sdl::point& camera);
			[[nodiscard]] sdl::point get_camera () const;
			[[nodiscard]] sdl::area_type get_dimension() const;
			void set_destination_point(const sdl::point& dst_point);
			[[nodiscard]] sdl::point get_destination_point() const;
			[[nodiscard]] sdl::rect get_destination_rect() const;
			[[nodiscard]] const texture_atlas& get_atlas() const;
			[[nodiscard]] sdl::renderer& get_renderer() const;
		private:
			void _update(std::chrono::milliseconds delta_time);
			void draw_layer (const tiled::image_layer& layer);
			void draw_layer (const tiled::color_layer& layer) const;
			void draw_layer (const tiled::tiles_layer& layer, std::chrono::milliseconds delta_time, std::set<tile>& updated_animations);
			void draw_layer (tiled::objects_layer& layer, std::chrono::milliseconds delta_time);

			sdl::renderer& m_renderer;
			sdl::area_type m_dimension;
			sdl::point     m_destination;
			sdl::texture m_target;
			texture_atlas* m_texture_atlas;
			const tiled::world_model* m_model;

			struct animation_data {
				explicit animation_data(const tiled::animation_sequence& seq);

				std::size_t m_current_frame;
				std::chrono::milliseconds m_time_in_current_frame;
				const tiled::animation_sequence& m_sequence;
			};

			std::map <tile, animation_data> m_animations;

			pixel_coord_t m_camera_x;
			pixel_coord_t m_camera_y;
	};
}

#endif
