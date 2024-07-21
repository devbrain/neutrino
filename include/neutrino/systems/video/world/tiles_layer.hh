//
// Created by igor on 7/4/24.
//

#ifndef  NEUTRINO_SYSTEMS_VIDEO_WORLD_TILES_LAYER_HH
#define  NEUTRINO_SYSTEMS_VIDEO_WORLD_TILES_LAYER_HH

#include <vector>
#include <neutrino/systems/video/tile.hh>
#include <neutrino/neutrino_export.hh>
namespace neutrino::tiled {
	class NEUTRINO_EXPORT tiles_layer {
		public:
			tiles_layer(tile_coord_t w,
			            tile_coord_t h,
			            pixel_coord_t tile_width,
			            pixel_coord_t tile_height);

			[[nodiscard]] tile_coord_t get_width() const;
			[[nodiscard]] tile_coord_t get_height() const;
			[[nodiscard]] pixel_coord_t get_tile_width() const;
			[[nodiscard]] pixel_coord_t get_tile_height() const;

			tile& at(tile_coord_t x, tile_coord_t y);
			[[nodiscard]] const tile& at(tile_coord_t x, tile_coord_t y) const;
			[[nodiscard]] static bool is_empty(const tile& t) ;
			void clear(tile_coord_t x, tile_coord_t y);

		private:
			tile_coord_t m_width;
			tile_coord_t m_height;
			pixel_coord_t m_tile_width;
			pixel_coord_t m_tile_height;
			std::vector <tile> m_tiles;
	};
}

#endif
