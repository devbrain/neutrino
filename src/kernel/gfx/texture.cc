//
// Created by igor on 07/05/2022.
//

#include "texture.hh"
#include <neutrino/hal/video/target_texture.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::kernel {

  texture::descr_t texture::dims (const image_loader_t&) {
    return std::monostate{};
  }

  texture::descr_t texture::dims (const lazy_tilesheet& lazy_ts) {
    const auto& td = std::get<1>(lazy_ts);
    if (const auto* ti = std::get_if<lazy_tilesheet_info> (&td)) {
      return get_tilesheet_coords (ti->image_width(), ti->image_height(), ti->info());
    }
    else {
      return *std::get_if<tilesheet_rects> (&td);
    }
  }

  texture::descr_t texture::dims (const image& img) {
    auto [w, h] = img.dimensions ();
    return math::rect{0, 0, (int) w, (int) h};
  }

  texture::descr_t texture::dims (const tilesheet& ts) {
    const auto& [s, td] = ts;
    if (const auto* ti = std::get_if<tilesheet_info> (&td)) {
      return get_tilesheet_coords (s, *ti);
    }
    else {
      return *std::get_if<tilesheet_rects> (&td);
    }
  }

  texture::texture (const image_loader_t& img_loader)
      : m_descr (dims (img_loader)),
        m_image_loader (img_loader) {

  }

  texture::texture (const lazy_tilesheet& lazy_ts)
    : m_descr (dims (lazy_ts)),
      m_image_loader (std::get<0>(lazy_ts)) {
      const auto& td = std::get<1>(lazy_ts);
      if (const auto* ti = std::get_if<lazy_tilesheet_info> (&td)) {
        m_expected_dims = std::make_pair (ti->image_width(), ti->image_height());
      }
  }



  texture::texture (hal::renderer& renderer, const image& img)
      : m_descr (dims (img)),
        m_texture (renderer, img) {

  }

  texture::texture (hal::renderer& renderer, const tilesheet& ts)
      : m_descr (dims (ts)),
        m_texture (renderer, std::get<0> (ts)) {
  }

  bool texture::is_tilesheet () const noexcept {
    return std::get_if<tilesheet_rects> (&m_descr) != nullptr;
  }

  std::size_t texture::num_tiles () const noexcept {
    if (const auto* ti = std::get_if<tilesheet_rects> (&m_descr)) {
      return ti->size ();
    }
    return 1;
  }

  math::rect texture::tile_rectangle (std::size_t id) const noexcept {
    if (const auto* ti = std::get_if<tilesheet_rects> (&m_descr)) {
      return ti->at (id);
    }
    ENFORCE(id == 0);
    const math::rect* r = std::get_if<math::rect> (&m_descr);
    ENFORCE(r != nullptr);
    return *r;
  }

  void texture::draw (hal::renderer& renderer, const math::rect& src_rect, const math::point2d& dst_top_left) const {
    math::rect dst_rect{dst_top_left, src_rect.dims};
    renderer.copy (m_texture, src_rect, dst_rect);
  }

  static math::dimension2di_t eval_transormr_dims(const math::dimension2di_t& orig, const rotation_info& ri) {
    if (ri.degree == 0) {
      if (ri.hflip) {
        return orig;
      } else if (ri.vflip) {
        return orig;
      }
    } else {
      if (ri.degree == 90) {
        return {orig[1], orig[0]};
      } else if (ri.degree == 180) {
        return orig;
      } else if (ri.degree == 270) {
        return {orig[1], orig[0]};
      }
    }
    RAISE_EX("unknown rotation");
  }

  void texture::draw(hal::renderer& renderer, const math::rect& src_rect,
                     const math::point2d& dst_top_left,
                     const math::rect& original_dims,
                     const rotation_info& ri) const {
    math::rect dst_rect{dst_top_left, src_rect.dims};
    if (ri.empty()) {
      renderer.copy (m_texture, src_rect, dst_rect);
    } else {
      auto new_dims = eval_transormr_dims (original_dims.dims, ri);
      if (new_dims != m_temp_dims) {
        hal::texture new_tex(renderer, renderer.get_pixel_format(), new_dims.x, new_dims.y, hal::texture::access::TARGET);
        swap(m_temp, new_tex);
        m_temp_dims = new_dims;
      }

      {
        hal::use_texture tt (renderer, m_temp);
        renderer.clear();
        math::rect dst(0,0, m_temp_dims.x, m_temp_dims.y);
        auto flip = hal::renderer::flip::NONE;
        if (ri.hflip) {
          flip = hal::renderer::flip::HORIZONTAL;
        } else if (ri.vflip) {
          flip = hal::renderer::flip::VERTICAL;
        }
        renderer.copy (m_texture, original_dims, dst, ri.degree, flip);
      }
      auto corrected_pos = src_rect.point - original_dims.point;
      math::rect s(corrected_pos.x, corrected_pos.y, original_dims.dims.x, original_dims.dims.y);
      renderer.copy (m_temp, s, dst_rect);
    }
  }

  void texture::convert(hal::renderer& renderer) {
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
          m_descr = dims(img);
        }
        hal::texture new_texture(renderer, img);
        swap(new_texture, m_texture);
      }
    }
  }
}
