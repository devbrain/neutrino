//
// Created by igor on 06/07/2021.
//

#ifndef NEUTRINO_TILED_WORLD_WORLD_HH
#define NEUTRINO_TILED_WORLD_WORLD_HH

#include <neutrino/tiled/world/tile_description.hh>
#include <neutrino/math/rect.hh>
#include <map>

namespace neutrino::tiled {

  class camera;

  class world_renderer;

  class world {
      friend class camera;

      friend class world_renderer;

    public:
      void clear ();

      [[nodiscard]] layer_id_t add (tiles_layer world_layer);
      [[nodiscard]] layer_id_t add (image_layer world_layer);

      [[nodiscard]] sprite_id_t add (sprite sprite_frames);
      [[nodiscard]] bound_sprite_id_t
      bind_sprite_to_layer (sprite_id_t sprite_id, layer_id_t layer_id, std::size_t current_frame, bool active);
      [[nodiscard]] bound_sprite_id_t
      bind_sprite_to_layer (sprite_id_t sprite_id, layer_id_t layer_id, std::size_t current_frame);
      [[nodiscard]] bound_sprite_id_t bind_sprite_to_layer (sprite_id_t sprite_id, layer_id_t layer_id);

      void sprite_state (bound_sprite_id_t bound_sprite_id, std::size_t current_frame, bool active);
      // frame, num of frames, is active
      [[nodiscard]] std::tuple<std::size_t, std::size_t, bool> sprite_state (bound_sprite_id_t bound_sprite_id) const;

      [[nodiscard]] math::dimension_t dims_in_pixels (layer_id_t layer_id) const;
      [[nodiscard]] math::dimension_t dims_in_tiles (layer_id_t layer_id) const;
      [[nodiscard]] math::dimension_t tile_dims (layer_id_t layer_id) const;
    private:
      std::vector<layer> m_layers;

      struct sprite_info {
        sprite_id_t sprite_id;
        std::size_t current;
        bool active;
      };

      std::vector<sprite> m_sprites;
      std::vector<sprite_info> m_bound_sprites;
      std::map<layer_id_t, std::vector<bound_sprite_id_t>> m_sprites_to_layers;
  };

}

#endif
