//
// Created by igor on 07/05/2022.
//

#include <iostream>
#include "tiled_image.hh"
#include <neutrino/hal/video/renderer_utils.hh>
#include <neutrino/utils/exception.hh>
#include <neutrino/assets/tiles/world/tilesheet_resource.hh>

namespace neutrino::kernel {


  tiled_image::tiled_image (assets::resource_id rid)
      : m_rid(rid) {
    auto r = assets::resource<assets::tilesheet_resource>::get(rid);
    if (r->rectangles) {
      m_is_image = false;
      m_rects = *r->rectangles;
    } else {
      auto [w,h] = r->surface.dimensions();
      m_rects.push_back (math::rect(0,0,w,h));
      m_is_image = true;
    }
  }



  tiled_image::tiled_image(hal::renderer& renderer, const hal::pixel_format& format, const math::dimension2di_t& canvas_dims)
  : m_texture (renderer, format, canvas_dims.x, canvas_dims.y, hal::texture::access::TARGET){
    m_texture.blend (hal::blend_mode::BLEND);
    m_rects.push_back(math::rect(0,0,canvas_dims.x, canvas_dims.y));
  }

  bool tiled_image::is_tilesheet () const noexcept {
    return !m_is_image;
  }

  std::size_t tiled_image::num_tiles () const noexcept {
    return m_rects.size();
  }

  math::rect tiled_image::tile_rectangle (assets::cell_id_t cell) const noexcept {
    const auto id = cell.value_of();
    if (!m_is_image) {
      return m_rects.at (id);
    }
    ENFORCE(id == 0);
    ENFORCE(m_rects.size() == 1);
    return m_rects[0];
  }

  void tiled_image::draw (hal::renderer& renderer, const math::rect& src_rect, const math::point2d& dst_top_left) const {
    math::rect dst_rect{dst_top_left, src_rect.dims};
    renderer.copy (m_texture, src_rect, dst_rect);
  }

  void tiled_image::convert(hal::renderer& renderer) {
    if (!m_texture) {
        auto r = assets::resource<assets::tilesheet_resource>::get(m_rid);
        hal::texture new_texture(renderer, r->surface);
        std::swap(new_texture, m_texture);
      }
    }

  void tiled_image::swap (tiled_image& other) noexcept {
    std::swap(m_rid, other.m_rid);
    std::swap(m_rects, other.m_rects);
    std::swap(m_texture, other.m_texture);
  }

  [[nodiscard]] const hal::texture& tiled_image::texture() const {
    return m_texture;
  }

  hal::texture& tiled_image::texture() {
    return m_texture;
  }
}
