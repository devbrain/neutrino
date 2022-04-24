//
// Created by igor on 19/04/2022.
//

#include <neutrino/kernel/gfx/grid_image.hh>
#include <neutrino/hal/video/image_loader.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::gfx {

  namespace {
    math::rect get_coords (unsigned id, unsigned canvas_w, unsigned canvas_h, const tile_sheet_info& inf)  {
      auto margin = inf.margin();
      auto spacing = inf.spacing();
      auto tilewidth = inf.tile_width();
      auto tileheight = inf.tile_height();
      auto offset_x = inf.offset_x();
      auto offset_y = inf.offset_y();

      unsigned width = (canvas_w - 2 * margin + spacing) / (tilewidth + spacing); // number of tiles
      unsigned height = (canvas_h - 2 * margin + spacing) / (tileheight + spacing); // number of tiles

      unsigned tu = id % width;
      unsigned tv = id / width;
      //ENFORCE(tv <= height);

      unsigned du = margin + tu * spacing + offset_x;
      unsigned dv = margin + tv * spacing + offset_y;
      //ENFORCE(((tu + 1) * tilewidth + du) <= canvas_w);
      //ENFORCE(((tv + 1) * tileheight + dv) <= canvas_h);

      return {(int) (tu * tilewidth + du), (int) (tv * tileheight + dv), (int) tilewidth, (int) tileheight};
    }
  }

  grid_image::grid_image(const std::filesystem::path& path, const tile_sheet_info& inf)
  : m_image(hal::load (path)) {
    setup_positions (inf);
  }

  grid_image::grid_image(std::istream& is, const tile_sheet_info& inf)
      : m_image(hal::load (is)) {
    setup_positions (inf);
  }

  grid_image::grid_image(hal::surface surface, const tile_sheet_info& inf)
      : m_image(std::move (surface)) {
    setup_positions (inf);
  }


  grid_image::grid_image(hal::surface surface, std::size_t num_tiles, const rect_func_t& rect_eval)
      : m_image(std::move (surface)) {
    setup_positions (num_tiles, rect_eval);
  }

  grid_image::grid_image(std::istream& is, std::size_t num_tiles, const rect_func_t& rect_eval)
  : m_image(hal::load (is)) {
    setup_positions (num_tiles, rect_eval);
  }

  grid_image::grid_image(const std::filesystem::path& path, std::size_t num_tiles, const rect_func_t& rect_eval)
      : m_image(hal::load (path)) {
    setup_positions (num_tiles, rect_eval);
  }

  std::vector<math::rect> grid_image::positions() const noexcept {
    return m_positions;
  }

  hal::texture grid_image::create_texture(const hal::renderer& renderer) const {
    return {renderer, m_image};
  }

  hal::texture grid_image::create_texture(const hal::renderer& renderer, hal::color key_color) const {
    auto old = m_image.color_key();
    const_cast<grid_image*>(this)->m_image.color_key(key_color);
    try {
      hal::texture t(renderer, m_image);
      if (old) {
        const_cast<grid_image*>(this)->m_image.color_key(*old);
      } else {
        const_cast<grid_image*>(this)->m_image.disable_color_key();
      }
      return t;
    } catch (std::exception&) {
      if (old) {
        const_cast<grid_image*>(this)->m_image.color_key(*old);
      } else {
        const_cast<grid_image*>(this)->m_image.disable_color_key();
      }
      throw ;
    }
  }

  void grid_image::setup_positions(const tile_sheet_info& inf) {
    auto n = inf.num_tiles();
    auto [canvas_w, canvas_h] = m_image.dimensions();
    if (n == 0) {
      n = (canvas_w / inf.tile_width()) * (canvas_h / inf.tile_height());
    }
    m_positions.reserve (n);
    for (std::size_t i=0; i<n; i++) {
      m_positions.emplace_back (get_coords (i, canvas_w, canvas_h, inf));
    }
  }

  void grid_image::setup_positions(std::size_t num_tiles, const rect_func_t& rect_eval) {
    auto [canvas_w, canvas_h] = m_image.dimensions();
    m_positions.reserve (num_tiles);
    for (std::size_t i=0; i<num_tiles; i++) {
      auto r = rect_eval(i);
      auto ep = r.point + r.dims;
      ENFORCE ((static_cast<unsigned>(ep[0]) <= canvas_w) && (static_cast<unsigned>(ep[1]) <= canvas_h));
      m_positions.emplace_back (r);
    }
  }
}
