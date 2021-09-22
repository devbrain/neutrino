//
// Created by igor on 28/08/2021.
//

#ifndef INCLUDE_NEUTRINO_TILED_TEXTURE_ATLAS_HH
#define INCLUDE_NEUTRINO_TILED_TEXTURE_ATLAS_HH

#include <vector>

#include <neutrino/math/rect.hh>
#include <neutrino/hal/video/texture.hh>
#include <neutrino/hal/video/surface.hh>
#include <neutrino/tiled/world/types.hh>

namespace neutrino::tiled {
  class texture_atlas_builder;

  class texture_atlas {
      friend class texture_atlas_builder;
    public:
      using sprite_coords_t = std::vector<math::rect>;
    public:
      texture_atlas(const texture_atlas& ) = delete;
      texture_atlas& operator = (const texture_atlas& ) = delete;

      texture_atlas(texture_atlas&& ) = default;
      texture_atlas& operator = (texture_atlas&& ) = default;
      /**
       * Adds texture with sprite coordinates
       * @param texture texture to add
       * @param coords vector of sprite coordinates
       */
      void add(hal::texture&& texture, sprite_coords_t&& coords);

      /**
       * Adds texture as the whole image
       * @param texture texture to add
       * @param width width of the image
       * @param height height of the image
       */
      void add(hal::texture&& texture, int width, int height);

      /**
       * returns the texture by id
       * @param id texture id obtained vi call to @see add
       * @param tile_id id of the tile in the texture
       * @return texture, and rectangle of the tile or throws exception
       */
      [[nodiscard]] std::tuple<const hal::texture&, math::rect> get(texture_id_t id, tile_id_t tile_id) const;
    private:
      texture_atlas() = default;
    private:
      using atlas_t = std::tuple<hal::texture, sprite_coords_t>;
      std::vector<atlas_t> m_atlas;
  };
}

#endif //INCLUDE_NEUTRINO_TILED_WORLD_TEXTURE_ATLAS_HH
