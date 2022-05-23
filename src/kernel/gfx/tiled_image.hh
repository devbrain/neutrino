//
// Created by igor on 07/05/2022.
//

#ifndef SRC_KERNEL_GFX_TILED_IMAGE_HH
#define SRC_KERNEL_GFX_TILED_IMAGE_HH

#include <variant>
#include <optional>


#include <neutrino/hal/video/texture.hh>
#include <neutrino/hal/video/renderer.hh>
#include <neutrino/kernel/rc/tilesheet_description.hh>
#include <neutrino/kernel/rc/types.hh>

namespace neutrino::kernel {
  class tiled_image {
    public:
      explicit tiled_image(const image_loader_t& img_loader);
      explicit tiled_image(const lazy_tilesheet& lazy_ts);
      tiled_image(hal::renderer& renderer, const image& img);
      tiled_image(hal::renderer& renderer, const tilesheet& ts);

      tiled_image(hal::renderer& renderer, const math::dimension2di_t& dims);

      [[nodiscard]] bool is_tilesheet() const noexcept;
      [[nodiscard]] std::size_t num_tiles() const noexcept;
      [[nodiscard]] math::rect  tile_rectangle(std::size_t id) const noexcept;
      void draw(hal::renderer& renderer, const math::rect& src_rect, const math::point2d& dst_top_left) const;
      void draw(hal::renderer& renderer,
                const math::rect& src_rect,
                const math::point2d& dst_top_left,
                const math::rect& original_dims,
                const rotation_info& ri) const;

      friend void swap(tiled_image& rhs, tiled_image& lhs) {
        std::swap(rhs.m_descr, lhs.m_descr);
        std::swap (rhs.m_image_loader, lhs.m_image_loader);
        swap(rhs.m_texture, lhs.m_texture);
      }

      void convert(hal::renderer& renderer);

      const hal::texture& texture() const {
        return m_texture;
      }

      hal::texture& texture() {
        return m_texture;
      }
    private:
      using descr_t = std::variant<std::monostate, math::rect, tilesheet_rects>;
      descr_t        m_descr;
      hal::texture   m_texture;
      image_loader_t m_image_loader;
      std::optional<std::pair<unsigned, unsigned>> m_expected_dims;

      mutable hal::texture m_temp;
      mutable math::dimension2di_t m_temp_dims;

    private:
      static descr_t eval_dimension_properties(const image_loader_t&);
      static descr_t eval_dimension_properties(const lazy_tilesheet& lazy_ts);
      static descr_t eval_dimension_properties(const image& img);
      static descr_t eval_dimension_properties(const tilesheet& ts);
      static descr_t eval_dimension_properties(const math::dimension2di_t& canvas_dims);
  };
}

#endif //SRC_KERNEL_GFX_TILED_IMAGE_HH
