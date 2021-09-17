//
// Created by igor on 28/08/2021.
//

#ifndef INCLUDE_NEUTRINO_TILED_WORLD_TEXTURE_ATLAS_HH
#define INCLUDE_NEUTRINO_TILED_WORLD_TEXTURE_ATLAS_HH

#include <vector>
#include <map>
#include <neutrino/math/rect.hh>
#include <neutrino/hal/video/texture.hh>
#include <neutrino/hal/video/surface.hh>
#include <neutrino/tiled/world/types.hh>

namespace neutrino::tiled {
  class texture_atlas {
    public:
      using sprite_coords_t = std::vector<math::rect>;
    public:
      /**
       * Adds texture with sprite coordinates
       * @param texture texture to add
       * @param coords vector of sprite coordinates
       * @return id of the added texture
       */
      [[nodiscard]] texture_id_t add(hal::texture&& texture, sprite_coords_t&& coords);

      /**
       * Adds texture as the whole image
       * @param texture texture to add
       * @param width width of the image
       * @param height height of the image
       * @return id of the texture
       */
      [[nodiscard]] texture_id_t add(hal::texture&& texture, int width, int height);

      /**
       * returns the texture by id
       * @param id texture id obtained vi call to @see add
       * @return texture or throws exception
       */
      [[nodiscard]] hal::texture& get(texture_id_t id);
      [[nodiscard]] const hal::texture& get(texture_id_t id) const;

      /**
       * checks if the given texture exists
       * @param id id of the texture obtained vi call to @see add
       * @return true iff the texture exists
       */
      [[nodiscard]] bool exists(texture_id_t id) const noexcept;

      /**
       * remove the texture by id. If no texture exists, nothing happens
       * @param id id of the texture obtained vi call to @see add
       */
      void remove(texture_id_t id);
    private:
      using atlas_t = std::tuple<hal::texture, sprite_coords_t>;
      std::map<texture_id_t, atlas_t> m_atlas;
  };
}

#endif //INCLUDE_NEUTRINO_TILED_WORLD_TEXTURE_ATLAS_HH
