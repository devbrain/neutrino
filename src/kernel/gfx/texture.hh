//
// Created by igor on 07/05/2022.
//

#ifndef SRC_KERNEL_GFX_TEXTURE_HH
#define SRC_KERNEL_GFX_TEXTURE_HH

#include <variant>
#include <optional>
#include <neutrino/hal/video/texture.hh>
#include <neutrino/hal/video/renderer.hh>
#include <neutrino/kernel/rc/tilesheet_description.hh>


namespace neutrino::kernel {
  class texture {
    public:
      explicit texture(const image_loader_t& img_loader);
      explicit texture(const lazy_tilesheet& lazy_ts);
      texture(hal::renderer& renderer, const image& img);
      texture(hal::renderer& renderer, const tilesheet& ts);

      [[nodiscard]] bool is_tilesheet() const noexcept;
      [[nodiscard]] std::size_t num_tiles() const noexcept;
      [[nodiscard]] math::rect  tile_rectangle(std::size_t id) const noexcept;
      void draw(hal::renderer& renderer, const math::rect& src_rect, const math::point2d& dst_top_left) const;

      friend void swap(texture& rhs, texture& lhs) {
        std::swap(rhs.m_descr, lhs.m_descr);
        std::swap (rhs.m_image_loader, lhs.m_image_loader);
        swap(rhs.m_texture, lhs.m_texture);
      }

      void convert(hal::renderer& renderer);
    private:
      using descr_t = std::variant<std::monostate, math::rect, tilesheet_rects>;
      descr_t        m_descr;
      hal::texture   m_texture;
      image_loader_t m_image_loader;
      std::optional<std::pair<unsigned, unsigned>> m_expected_dims;
    private:
      static descr_t dims(const image_loader_t&);
      static descr_t dims(const lazy_tilesheet& lazy_ts);
      static descr_t dims(const image& img);
      static descr_t dims(const tilesheet& ts);
  };
}

#endif //SRC_KERNEL_GFX_TEXTURE_HH
