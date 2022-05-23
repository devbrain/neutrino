//
// Created by igor on 07/05/2022.
//

#include <iostream>
#include "tiled_image.hh"
#include <neutrino/hal/video/renderer_utils.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::kernel {

  tiled_image::descr_t tiled_image::eval_dimension_properties (const image_loader_t&) {
    return std::monostate{};
  }

  tiled_image::descr_t tiled_image::eval_dimension_properties (const lazy_tilesheet& lazy_ts) {
    const auto& td = std::get<1>(lazy_ts);
    if (const auto* ti = std::get_if<lazy_tilesheet_info> (&td)) {
      return get_tilesheet_coords (ti->image_width(), ti->image_height(), ti->info());
    }
    else {
      return *std::get_if<tilesheet_rects> (&td);
    }
  }

  tiled_image::descr_t tiled_image::eval_dimension_properties (const image& img) {
    auto [w, h] = img.dimensions ();
    return math::rect{0, 0, (int) w, (int) h};
  }

  tiled_image::descr_t tiled_image::eval_dimension_properties (const tilesheet& ts) {
    const auto& [s, td] = ts;
    if (const auto* ti = std::get_if<tilesheet_info> (&td)) {
      return get_tilesheet_coords (s, *ti);
    }
    else {
      return *std::get_if<tilesheet_rects> (&td);
    }
  }

  tiled_image::descr_t tiled_image::eval_dimension_properties(const math::dimension2di_t& canvas_dims) {
    tilesheet_rects result;
    result.emplace_back (0,0, canvas_dims.x, canvas_dims.y);
    return result;
  }

  tiled_image::tiled_image (const image_loader_t& img_loader)
      : m_descr (eval_dimension_properties (img_loader)),
        m_image_loader (img_loader) {
  }

  tiled_image::tiled_image (const lazy_tilesheet& lazy_ts)
    : m_descr (eval_dimension_properties (lazy_ts)),
      m_image_loader (std::get<0>(lazy_ts)) {
      const auto& td = std::get<1>(lazy_ts);
      if (const auto* ti = std::get_if<lazy_tilesheet_info> (&td)) {
        m_expected_dims = std::make_pair (ti->image_width(), ti->image_height());
      }
  }

  tiled_image::tiled_image (hal::renderer& renderer, const image& img)
      : m_descr (eval_dimension_properties (img)),
        m_texture (renderer, img) {

  }

  tiled_image::tiled_image (hal::renderer& renderer, const tilesheet& ts)
      : m_descr (eval_dimension_properties (ts)),
        m_texture (renderer, std::get<0> (ts)) {
  }

  tiled_image::tiled_image(hal::renderer& renderer, const math::dimension2di_t& canvas_dims)
  : m_descr (eval_dimension_properties (canvas_dims)),
    m_texture (renderer, renderer.get_pixel_format(), canvas_dims.x, canvas_dims.y, hal::texture::access::TARGET){
    hal::use_color uc(renderer, 0,0,0,0);
    m_texture.blend (hal::blend_mode::BLEND);
  }

  bool tiled_image::is_tilesheet () const noexcept {
    return std::get_if<tilesheet_rects> (&m_descr) != nullptr;
  }

  std::size_t tiled_image::num_tiles () const noexcept {
    if (const auto* ti = std::get_if<tilesheet_rects> (&m_descr)) {
      return ti->size ();
    }
    return 1;
  }

  math::rect tiled_image::tile_rectangle (std::size_t id) const noexcept {
    if (const auto* ti = std::get_if<tilesheet_rects> (&m_descr)) {
      return ti->at (id);
    }
    ENFORCE(id == 0);
    const math::rect* r = std::get_if<math::rect> (&m_descr);
    ENFORCE(r != nullptr);
    return *r;
  }

  void tiled_image::draw (hal::renderer& renderer, const math::rect& src_rect, const math::point2d& dst_top_left) const {
    math::rect dst_rect{dst_top_left, src_rect.dims};
    auto b = renderer.blend();
    renderer.blend (hal::blend_mode::BLEND);
    renderer.copy (m_texture, src_rect, dst_rect);
    renderer.blend(b);
  }

  void tiled_image::convert(hal::renderer& renderer) {
    if (!m_texture) {
      if (m_image_loader) {
        auto img = m_image_loader();
        if (m_expected_dims) {
          auto [expected_width, expected_height] = *m_expected_dims;
          auto [actual_width, actual_height] = img.dimensions();
          ENFORCE(expected_width == actual_width);
          ENFORCE(expected_height == actual_height);
        }
        if (std::get_if<std::monostate>(&m_descr)) {
          m_descr = eval_dimension_properties (img);
        }
        hal::texture new_texture(renderer, img);
        swap(new_texture, m_texture);
        m_texture.blend (hal::blend_mode::BLEND);
      }
    }
  }
}
