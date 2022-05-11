//
// Created by igor on 08/05/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_GFX_TEXTURE_ATLAS_HH
#define INCLUDE_NEUTRINO_KERNEL_GFX_TEXTURE_ATLAS_HH

#include <neutrino/kernel/rc/tilesheet_description.hh>
#include <neutrino/kernel/rc/types.hh>
#include <neutrino/kernel/rc/image.hh>
#include <neutrino/utils/spimpl.h>

#include <neutrino/hal/video/renderer.hh>
#include <neutrino/math/rect.hh>

namespace neutrino::kernel {

  struct tile_data {
    tile_id_t tile_id;
    math::rect src;
  };

  class texture_atlas {
    public:
      texture_atlas ();
      ~texture_atlas();

      [[nodiscard]] atlas_id_t add(hal::renderer& renderer, const image& img);
      [[nodiscard]] atlas_id_t add(hal::renderer& renderer, const tilesheet& ts);

      [[nodiscard]] atlas_id_t add(lazy_tilesheet& ts);
      [[nodiscard]] atlas_id_t add(image_loader_t& loader);

      void replace(atlas_id_t atlas_id, hal::renderer& renderer, const image& img);
      void replace(atlas_id_t atlas_id, hal::renderer& renderer, const tilesheet & img);
      void replace(atlas_id_t atlas_id, lazy_tilesheet& lt);
      void replace(atlas_id_t atlas_id, image_loader_t& img_ldr);
      void convert_images(hal::renderer& renderer);

      [[nodiscard]] bool is_tilesheet(atlas_id_t atlas_id) const noexcept;
      [[nodiscard]] cell_id_t num_of_tiles(atlas_id_t atlas_id) const noexcept;
      [[nodiscard]] tile_data  tile_rectangle(atlas_id_t atlas_id, cell_id_t tile_id) const noexcept;
      [[nodiscard]] tile_data  tile_rectangle(tile_id_t tile) const noexcept;
      void draw(hal::renderer& renderer, const tile_data& tile, const math::point2d& dst_top_left) const;
    private:
      struct impl;
      spimpl::unique_impl_ptr<impl> m_pimpl;
  };

  inline
  tile_data texture_atlas::tile_rectangle(tile_id_t tile) const noexcept {
    return tile_rectangle (tile.atlas_id, tile.cell_id);
  }
}

#endif //INCLUDE_NEUTRINO_KERNEL_GFX_TEXTURE_ATLAS_HH
