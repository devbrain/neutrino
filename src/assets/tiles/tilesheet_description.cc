//
// Created by igor on 07/05/2022.
//
#include <neutrino/assets/tiles/tilesheet_description.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::assets {
  tilesheet_info::tilesheet_info (unsigned tile_width,
                                    unsigned tile_height,
                                    unsigned spacing,
                                    unsigned margin,
                                    unsigned offset_x,
                                    unsigned offset_y,
                                    std::size_t num_tiles)
      : m_tile_width{tile_width}, m_tile_height{tile_height}, m_spacing{spacing},
        m_margin{margin}, m_offset_x(offset_x), m_offset_y(offset_y), m_num_tiles(num_tiles) {
  }

  unsigned tilesheet_info::tile_width () const noexcept {
    return m_tile_width;
  }

  unsigned tilesheet_info::tile_height () const noexcept {
    return m_tile_height;
  }

  unsigned tilesheet_info::spacing () const noexcept {
    return m_spacing;
  }

  unsigned tilesheet_info::margin () const noexcept {
    return m_margin;
  }

  unsigned int tilesheet_info::offset_x () const noexcept {
    return m_offset_x;
  }

  unsigned int tilesheet_info::offset_y () const noexcept {
    return m_offset_y;
  }

  size_t tilesheet_info::num_tiles () const noexcept {
    return m_num_tiles;
  }

  math::rect get_tilesheet_coords (unsigned id, unsigned canvas_w, unsigned canvas_h, const tilesheet_info& inf) {
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

  math::rect get_tilesheet_coords (unsigned id, const hal::surface& ts_image, const tilesheet_info& inf) {
    auto [w,h] = ts_image.dimensions();
    return get_tilesheet_coords (id, (int)w, (int)h, inf);
  }

  tilesheet_rects get_tilesheet_coords(unsigned canvas_w, unsigned canvas_h, const tilesheet_info& inf) {
    tilesheet_rects result;
    result.reserve (inf.num_tiles());
    for (std::size_t id=0; id<inf.num_tiles(); id++) {
      result.push_back (get_tilesheet_coords (id, canvas_w, canvas_h, inf));
    }
    return result;
  }

  tilesheet_rects get_tilesheet_coords(const hal::surface& ts_image, const tilesheet_info& inf) {
    auto [w,h] = ts_image.dimensions();
    return get_tilesheet_coords (w, h, inf);
  }

  tilesheet_rects get_tilesheet_coords(const image& ts_image, const tilesheet_info& inf) {
    auto dims = ts_image.dimensions();
    ENFORCE(dims); //-V1044
    return get_tilesheet_coords (dims->x, dims->y, inf);
  }


  lazy_tilesheet_info::lazy_tilesheet_info(unsigned image_width, unsigned image_height, tilesheet_info inf)
  : m_width(image_width),
    m_height(image_height),
    m_info (inf) {}

  unsigned lazy_tilesheet_info::image_width() const noexcept {
    return m_width;
  }

  unsigned lazy_tilesheet_info::image_height() const noexcept {
    return m_height;
  }

  const tilesheet_info& lazy_tilesheet_info::info() const noexcept {
    return m_info;
  }
}
