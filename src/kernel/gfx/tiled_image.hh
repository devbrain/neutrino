//
// Created by igor on 07/05/2022.
//

#ifndef SRC_KERNEL_GFX_TILED_IMAGE_HH
#define SRC_KERNEL_GFX_TILED_IMAGE_HH

#include <variant>
#include <optional>


#include <neutrino/hal/video/texture.hh>
#include <neutrino/hal/video/renderer.hh>
#include "neutrino/assets/tiles/tilesheet_description.hh"
#include "neutrino/assets/tiles/types.hh"

namespace neutrino::kernel {
  class tiled_image {
    public:
      explicit tiled_image(const assets::image& img_loader);
      explicit tiled_image(const assets::lazy_tilesheet& lazy_ts);

      tiled_image(hal::renderer& renderer, const hal::surface& img);
      tiled_image(hal::renderer& renderer, const assets::tilesheet& ts);

      tiled_image(hal::renderer& renderer, const hal::pixel_format& format, const math::dimension2di_t& dims);

      [[nodiscard]] bool is_tilesheet() const noexcept;
      [[nodiscard]] std::size_t num_tiles() const noexcept;
      [[nodiscard]] math::rect  tile_rectangle(assets::cell_id_t id) const noexcept;
      void draw(hal::renderer& renderer, const math::rect& src_rect, const math::point2d& dst_top_left) const;

      void swap(tiled_image& other) noexcept;

      void convert(hal::renderer& renderer);

      [[nodiscard]] const hal::texture& texture() const {
        return m_texture;
      }

      hal::texture& texture() {
        return m_texture;
      }
    private:
      using descr_t = std::variant<std::monostate, math::rect, assets::tilesheet_rects>;
      descr_t        m_descr;
      hal::texture   m_texture;
      assets::image  m_image_loader;
      std::optional<std::pair<unsigned, unsigned>> m_expected_dims;
    private:
      static descr_t eval_dimension_properties(const assets::image&);
      static descr_t eval_dimension_properties(const assets::lazy_tilesheet& lazy_ts);
      static descr_t eval_dimension_properties(const hal::surface& img);
      static descr_t eval_dimension_properties(const assets::tilesheet& ts);
      static descr_t eval_dimension_properties(const math::dimension2di_t& canvas_dims);
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
