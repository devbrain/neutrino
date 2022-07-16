//
// Created by igor on 07/05/2022.
//

#ifndef SRC_KERNEL_GFX_TILED_IMAGE_HH
#define SRC_KERNEL_GFX_TILED_IMAGE_HH

#include <utility>
#include <variant>
#include <optional>



#include <neutrino/hal/video/texture.hh>
#include <neutrino/hal/video/renderer.hh>
#include "neutrino/assets/tiles/tilesheet_description.hh"
#include "neutrino/assets/resources/resource_id.hh"
#include "neutrino/assets/tiles/types.hh"

namespace neutrino::kernel {
  class tiled_image {
    public:
      explicit tiled_image(assets::resource_id rid);

      tiled_image(hal::renderer& renderer, const hal::pixel_format& format, const math::dimension2di_t& dims);

      [[nodiscard]] bool is_tilesheet() const noexcept;
      [[nodiscard]] std::size_t num_tiles() const noexcept;
      [[nodiscard]] math::rect  tile_rectangle(assets::cell_id_t id) const noexcept;
      void draw(hal::renderer& renderer, const math::rect& src_rect, const math::point2d& dst_top_left) const;

      void swap(tiled_image& other) noexcept;

      void convert(hal::renderer& renderer);

      [[nodiscard]] const hal::texture& texture() const;
      hal::texture& texture();
    private:
      assets::resource_id     m_rid;
      assets::tilesheet_rects m_rects;
      bool                    m_is_image;
      hal::texture            m_texture;


  };
}

namespace std {

  template<>
  inline
  void swap(neutrino::kernel::tiled_image& rhs, neutrino::kernel::tiled_image& lhs) noexcept {
    rhs.swap (lhs);
  }

}

#endif //SRC_KERNEL_GFX_TILED_IMAGE_HH
