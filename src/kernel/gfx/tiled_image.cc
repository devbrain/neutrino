//
// Created by igor on 07/05/2022.
//

#include <iostream>
#include "tiled_image.hh"
#include <neutrino/hal/video/renderer_utils.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::kernel {

  tiled_image::descr_t tiled_image::eval_dimension_properties (const assets::lazy_image_loader&) {
    return std::monostate{};
  }

  tiled_image::descr_t tiled_image::eval_dimension_properties (const assets::lazy_tilesheet& lazy_ts) {
    const auto& td = std::get<1>(lazy_ts);
    if (const auto* ti = std::get_if<assets::lazy_tilesheet_info> (&td)) {
      return get_tilesheet_coords (ti->image_width(), ti->image_height(), ti->info());
    }
    else {
      return *std::get_if<assets::tilesheet_rects> (&td);
    }
  }

  tiled_image::descr_t tiled_image::eval_dimension_properties (const hal::surface& img) {
    auto [w, h] = img.dimensions ();
    return math::rect{0, 0, (int) w, (int) h};
  }

  tiled_image::descr_t tiled_image::eval_dimension_properties (const assets::tilesheet& ts) {
    const auto& [s, td] = ts;
    if (const auto* ti = std::get_if<assets::tilesheet_info> (&td)) {
      return get_tilesheet_coords (s, *ti);
    }
    else {
      return *std::get_if<assets::tilesheet_rects> (&td);
    }
  }

  tiled_image::descr_t tiled_image::eval_dimension_properties(const math::dimension2di_t& canvas_dims) {
    assets::tilesheet_rects result;
    result.emplace_back (0,0, canvas_dims.x, canvas_dims.y);
    return result;
  }

  tiled_image::tiled_image (const assets::lazy_image_loader& img_loader)
      : m_descr (eval_dimension_properties (img_loader)),
        m_image_loader (img_loader) {
  }

  tiled_image::tiled_image (const assets::lazy_tilesheet& lazy_ts)
    : m_descr (eval_dimension_properties (lazy_ts)),
      m_image_loader (std::get<0>(lazy_ts)) {
      const auto& td = std::get<1>(lazy_ts);
      if (const auto* ti = std::get_if<assets::lazy_tilesheet_info> (&td)) {
        m_expected_dims = std::make_pair (ti->image_width(), ti->image_height());
      }
  }

  tiled_image::tiled_image (hal::renderer& renderer, const hal::surface& img)
      : m_descr (eval_dimension_properties (img)),
        m_texture (renderer, img) {
  }

  tiled_image::tiled_image (hal::renderer& renderer, const assets::tilesheet& ts)
      : m_descr (eval_dimension_properties (ts)),
        m_texture (renderer, std::get<0> (ts)) {
  }

  tiled_image::tiled_image(hal::renderer& renderer, const hal::pixel_format& format, const math::dimension2di_t& canvas_dims)
  : m_descr (eval_dimension_properties (canvas_dims)),
    m_texture (renderer, format, canvas_dims.x, canvas_dims.y, hal::texture::access::TARGET){
    m_texture.blend (hal::blend_mode::BLEND);
  }

  bool tiled_image::is_tilesheet () const noexcept {
    return std::get_if<assets::tilesheet_rects> (&m_descr) != nullptr;
  }

  std::size_t tiled_image::num_tiles () const noexcept {
    if (const auto* ti = std::get_if<assets::tilesheet_rects> (&m_descr)) {
      return ti->size ();
    }
    return 1;
  }

  math::rect tiled_image::tile_rectangle (assets::cell_id_t cell) const noexcept {
    const auto id = cell.value_of();
    if (const auto* ti = std::get_if<assets::tilesheet_rects> (&m_descr)) {
      return ti->at (id);
    }
    ENFORCE(id == 0); //-V1044
    const math::rect* r = std::get_if<math::rect> (&m_descr);
    ENFORCE(r != nullptr); //-V1044
    return *r;
  }

  void tiled_image::draw (hal::renderer& renderer, const math::rect& src_rect, const math::point2d& dst_top_left) const {
    math::rect dst_rect{dst_top_left, src_rect.dims};
    renderer.copy (m_texture, src_rect, dst_rect);
  }

  void tiled_image::convert(hal::renderer& renderer) {
    if (!m_texture) {
      if (m_image_loader) {
        auto img = m_image_loader.load();
        if (m_expected_dims) {
          auto [expected_width, expected_height] = *m_expected_dims;
          auto [actual_width, actual_height] = img.dimensions();
          ENFORCE(expected_width == actual_width); //-V1044
          ENFORCE(expected_height == actual_height); //-V1044
        }
        if (std::get_if<std::monostate>(&m_descr)) {
          m_descr = eval_dimension_properties (img);
        }
        hal::texture new_texture(renderer, img);
        std::swap(new_texture, m_texture);
      }
    }
  }

  void tiled_image::swap (tiled_image& other) noexcept {
    std::swap(m_descr, other.m_descr);
    std::swap (m_image_loader, other.m_image_loader);
    std::swap(m_texture, other.m_texture);
  }
}
