//
// Created by igor on 7/15/24.
//

#ifndef NEUTRINO_MODULES_VIDEO_TEXTURE_ATLAS_HH
#define NEUTRINO_MODULES_VIDEO_TEXTURE_ATLAS_HH

#include <tuple>
#include <vector>
#include <map>

#include <sdlpp/sdlpp.hh>

#include <neutrino/modules/video/types.hh>
#include <neutrino/modules/video/tile.hh>
#include <neutrino/neutrino_export.hh>
#include <assets/resources/tileset/tileset.hh>
#include <neutrino/modules/video/world/world_serializer.hh>

namespace neutrino {
	class NEUTRINO_EXPORT texture_atlas {
		friend NEUTRINO_EXPORT void serialize_tmx(const std::filesystem::path& out_dir,
					   const tiled::world_model& model,
					   const texture_atlas& atlas,
					   const sdl::renderer& renderer);
		public:
			texture_id_t add(sdl::texture&& texture);
			texture_id_t add(sdl::texture&& texture, std::vector <sdl::rect>&& rects);

			texture_id_t add(sdl::surface&& srf, const sdl::renderer& renderer);
			texture_id_t add(sdl::surface&& srf, std::vector <sdl::rect>&& rects, const sdl::renderer& renderer);
			texture_id_t add(std::tuple<sdl::surface, std::vector <sdl::rect>>&& material, const sdl::renderer& renderer);
			texture_id_t add(assets::tileset&& tileset, const sdl::renderer& renderer);


			[[nodiscard]] std::tuple <const sdl::texture*, sdl::rect> get(texture_id_t tid) const;
			[[nodiscard]] std::tuple <const sdl::texture*, sdl::rect> get(const tile& tlid) const;

			void remove(texture_id_t tid);

		private:
			struct tiles_map {
				tiles_map(std::vector <sdl::rect>&& rects, sdl::texture&& texture);
				tiles_map(tiles_map&&) noexcept = default;
				std::vector <sdl::rect> m_rects; // tiles
				sdl::texture m_texture; // texture itself
				sdl::rect m_dims; // dimensions of the texture
			};

			std::map <texture_id_t, tiles_map> m_textures;
	};
}

#endif
