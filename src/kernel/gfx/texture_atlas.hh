//
// Created by igor on 08/05/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_GFX_TEXTURE_ATLAS_HH
#define INCLUDE_NEUTRINO_KERNEL_GFX_TEXTURE_ATLAS_HH


#include <neutrino/assets/tiles/image_atlas.hh>
#include <neutrino/assets/tiles/types.hh>
#include <neutrino/assets/tiles/tile_handle.hh>
#include <neutrino/utils/spimpl.h>

#include <neutrino/hal/video/renderer.hh>
#include <neutrino/math/rect.hh>

namespace neutrino::kernel {

  class texture_atlas {
    public:
      texture_atlas ();
      ~texture_atlas();

      void assign(const assets::image_atlas& atlas);

      void convert_images(hal::renderer& renderer);

      [[nodiscard]] bool is_tilesheet(assets::atlas_id_t atlas_id) const noexcept;
      [[nodiscard]] math::rect tile_rectangle(const assets::tile_handle& th) const noexcept;
      void draw(hal::renderer& renderer, const assets::tile_handle& tile, const math::rect& src, const math::point2d& dst_top_left) const;
    private:
      [[nodiscard]] assets::atlas_id_t add(hal::renderer& renderer, const hal::surface& img);
      [[nodiscard]] assets::atlas_id_t add(hal::renderer& renderer, const assets::tilesheet& ts);
      [[nodiscard]] assets::atlas_id_t add(const assets::lazy_tilesheet& ts);
      [[nodiscard]] assets::atlas_id_t add(const assets::lazy_image_loader& loader);
      [[nodiscard]] assets::atlas_id_t add(const assets::color& bgcolor);
    private:
      void replace(assets::atlas_id_t atlas_id, hal::renderer& renderer, const hal::surface& img);
      void replace(assets::atlas_id_t atlas_id, hal::renderer& renderer, const assets::tilesheet & img);
      void replace(assets::atlas_id_t atlas_id, const assets::lazy_tilesheet& lt);
      void replace(assets::atlas_id_t atlas_id, const assets::lazy_image_loader& img_ldr);
      void replace(assets::atlas_id_t atlas_id, const assets::color& bg_color);

      void set(assets::atlas_id_t atlas_id, hal::renderer& renderer, const hal::surface& img);
      void set(assets::atlas_id_t atlas_id, hal::renderer& renderer, const assets::tilesheet & img);
      void set(assets::atlas_id_t atlas_id, const assets::lazy_tilesheet& lt);
      void set(assets::atlas_id_t atlas_id, const assets::lazy_image_loader& img_ldr);
      void set(assets::atlas_id_t atlas_id, const assets::color& bg_color);
    private:
      struct impl;
      spimpl::unique_impl_ptr<impl> m_pimpl;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_GFX_TEXTURE_ATLAS_HH
