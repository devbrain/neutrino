//
// Created by igor on 07/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_TILES_TILESHEET_DESCRIPTION_HH
#define INCLUDE_NEUTRINO_ASSETS_TILES_TILESHEET_DESCRIPTION_HH

#include <vector>
#include <tuple>
#include <variant>
#include <functional>
#include <neutrino/math/rect.hh>
#include "neutrino/assets/image/lazy_image_loader.hh"

namespace neutrino::assets {
  using tilesheet_rects = std::vector<math::rect>;

  class tilesheet_info {
    public:
      tilesheet_info (unsigned tile_width,
                       unsigned tile_height,
                       unsigned spacing,
                       unsigned margin,
                       unsigned offset_x,
                       unsigned offset_y,
                       std::size_t num_tiles);


      [[nodiscard]] unsigned tile_width () const noexcept;
      [[nodiscard]] unsigned tile_height () const noexcept;
      [[nodiscard]] unsigned spacing () const noexcept;
      [[nodiscard]] unsigned margin () const noexcept;
      [[nodiscard]] unsigned offset_x () const noexcept;
      [[nodiscard]] unsigned offset_y () const noexcept;
      [[nodiscard]] std::size_t num_tiles () const noexcept;
    private:
      unsigned m_tile_width;
      unsigned m_tile_height;
      unsigned m_spacing; // The spacing in pixels between the tiles in this tileset
      unsigned m_margin;  // The margin around the tiles in this tileset
      unsigned m_offset_x;
      unsigned m_offset_y;
      std::size_t m_num_tiles;
  };

  math::rect get_tilesheet_coords (unsigned id, unsigned canvas_w, unsigned canvas_h, const tilesheet_info& inf);
  math::rect get_tilesheet_coords (unsigned id, const hal::surface& ts_image, const tilesheet_info& inf);
  math::rect get_tilesheet_coords (unsigned id, const lazy_image_loader& ts_image, const tilesheet_info& inf);

  tilesheet_rects get_tilesheet_coords(unsigned canvas_w, unsigned canvas_h, const tilesheet_info& inf);
  tilesheet_rects get_tilesheet_coords(const hal::surface& ts_image, const tilesheet_info& inf);
  tilesheet_rects get_tilesheet_coords(const lazy_image_loader& ts_image, const tilesheet_info& inf);


  using tilesheet = std::tuple<hal::surface, std::variant<tilesheet_rects, tilesheet_info>>;


  class lazy_tilesheet_info {
    public:
      lazy_tilesheet_info(unsigned image_width, unsigned image_height, tilesheet_info inf);

      [[nodiscard]] unsigned image_width() const noexcept;
      [[nodiscard]] unsigned image_height() const noexcept;
      [[nodiscard]] const tilesheet_info& info() const noexcept;
    private:
      unsigned m_width;
      unsigned m_height;
      tilesheet_info m_info;
  };

  using lazy_tilesheet = std::tuple<lazy_image_loader, std::variant<tilesheet_rects, lazy_tilesheet_info>>;

  inline
  tilesheet make_tilesheet(hal::surface img, tilesheet_rects td) {
    return std::make_tuple<hal::surface, std::variant<tilesheet_rects, tilesheet_info>>(std::move(img), std::move(td));
  }

  inline
  tilesheet make_tilesheet(std::tuple<hal::surface, tilesheet_rects> d) {
    return std::make_tuple<hal::surface, std::variant<tilesheet_rects, tilesheet_info>>(std::move(std::get<0>(d)),
                                                                                 std::move(std::get<1>(d)));
  }

  inline
  tilesheet make_tilesheet(std::pair<hal::surface, tilesheet_rects> d) {
    return std::make_tuple<hal::surface, std::variant<tilesheet_rects, tilesheet_info>>(std::move(std::get<0>(d)),
                                                                                 std::move(std::get<1>(d)));
  }

  inline
  tilesheet make_tilesheet(hal::surface img, tilesheet_info tdi) {
    return std::make_tuple<hal::surface, std::variant<tilesheet_rects, tilesheet_info>>(std::move(img), tdi);
  }

  inline
  tilesheet make_tilesheet(std::tuple<hal::surface,tilesheet_info> d) {
    return std::make_tuple<hal::surface, std::variant<tilesheet_rects, tilesheet_info>>(std::move(std::get<0>(d)),
                                                                                 std::get<1>(d));
  }

  inline
  tilesheet make_tilesheet(std::pair<hal::surface,tilesheet_info> d) {
    return std::make_tuple<hal::surface, std::variant<tilesheet_rects, tilesheet_info>>(std::move(std::get<0>(d)),
                                                                                 std::get<1>(d));
  }


  inline
  lazy_tilesheet make_tilesheet(lazy_image_loader img, tilesheet_rects td) {
    return std::make_tuple<lazy_image_loader, std::variant<tilesheet_rects, lazy_tilesheet_info>>(std::move(img), std::move(td));
  }

  inline
  lazy_tilesheet make_tilesheet(std::tuple<lazy_image_loader, tilesheet_rects> d) {
    return std::make_tuple<lazy_image_loader, std::variant<tilesheet_rects, lazy_tilesheet_info>>(std::move(std::get<0>(d)),
                                                                                 std::move(std::get<1>(d)));
  }

  inline
  lazy_tilesheet make_tilesheet(std::pair<lazy_image_loader, tilesheet_rects> d) {
    return std::make_tuple<lazy_image_loader, std::variant<tilesheet_rects, lazy_tilesheet_info>>(std::move(std::get<0>(d)),
                                                                                 std::move(std::get<1>(d)));
  }

  inline
  lazy_tilesheet make_tilesheet(lazy_image_loader img, lazy_tilesheet_info tdi) {
    return std::make_tuple<lazy_image_loader, std::variant<tilesheet_rects, lazy_tilesheet_info>>(std::move(img), tdi);
  }

  inline
  lazy_tilesheet make_tilesheet(std::tuple<lazy_image_loader,lazy_tilesheet_info> d) {
    return std::make_tuple<lazy_image_loader, std::variant<tilesheet_rects, lazy_tilesheet_info>>(std::move(std::get<0>(d)),
                                                                                 std::get<1>(d));
  }

  inline
  lazy_tilesheet make_tilesheet(std::pair<lazy_image_loader,lazy_tilesheet_info> d) {
    return std::make_tuple<lazy_image_loader, std::variant<tilesheet_rects, lazy_tilesheet_info>>(std::move(std::get<0>(d)),
                                                                                     std::get<1>(d));
  }
}

#endif //INCLUDE_NEUTRINO_ASSETS_TILES_TILESHEET_DESCRIPTION_HH
