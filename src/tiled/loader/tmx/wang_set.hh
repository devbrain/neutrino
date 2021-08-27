//
// Created by igor on 19/08/2021.
//

#ifndef NEUTRINO_WANG_SET_HH
#define NEUTRINO_WANG_SET_HH

#include <vector>
#include <string>
#include <array>

#include "component.hh"
#include "reader.hh"
#include "color.hh"

namespace neutrino::tiled::tmx {

  class wang_color : public component {
    public:
      static wang_color parse (const reader &elt);

      wang_color (colori color, std::string name, int cl, double p)
          : m_color (color), m_name (std::move (name)), m_cell (cl), m_prob (p) {
      }

      [[nodiscard]] colori color () const noexcept {
        return m_color;
      }

      [[nodiscard]] std::string name () const noexcept {
        return m_name;
      }

      [[nodiscard]] int local_tile () const noexcept {
        return m_cell;
      }

      [[nodiscard]] double prob () const noexcept {
        return m_prob;
      }
    private:
      colori m_color;
      std::string m_name;
      int m_cell;
      double m_prob;
  };

  class wang_tile {
    public:
      enum index_t {
        TOP = 0,
        TOP_RIGHT = 1,
        RIGHT = 2,
        BOTTOM_RIGHT = 3,
        BOTTOM = 4,
        BOTTOM_LEFT = 5,
        LEFT = 6,
        TOP_LEFT = 7
      };
      using wang_data_t = std::array<unsigned, 8>;
    public:
      static wang_tile parse (const reader &elt);
      static wang_data_t parse_corners (const reader &elt);

      wang_tile (wang_data_t wang_id,
                 unsigned gid,
                 bool hflip,
                 bool vflip,
                 bool dflip)
          : m_wang_id (wang_id),
            m_gid (gid),
            m_hflip (hflip),
            m_vflip (vflip),
            m_dflip (dflip) {

      }

      [[nodiscard]] wang_data_t wang_id () const noexcept {
        return m_wang_id;
      }

      [[nodiscard]] unsigned gid () const noexcept {
        return m_gid;
      }

      [[nodiscard]] bool hor_flipped () const noexcept {
        return m_hflip;
      }

      [[nodiscard]] bool vert_flipped () const noexcept {
        return m_vflip;
      }

      [[nodiscard]] bool diag_flipped () const noexcept {
        return m_dflip;
      }
    private:
      wang_data_t m_wang_id;
      unsigned m_gid;
      bool m_hflip;
      bool m_vflip;
      bool m_dflip;
  };

  class wang_set : public component {
    public:
      static wang_set parse (const reader &elt);

      wang_set (std::string name, int tile_id)
          : m_name (std::move (name)),
            m_cell (tile_id) {
      }

      [[nodiscard]] std::string name () const noexcept {
        return m_name;
      }
      [[nodiscard]] int local_tile () const noexcept {
        return m_cell;
      }

      void add_color (wang_color wc) {
        m_colors.emplace_back (std::move (wc));
      }

      [[nodiscard]] const std::vector<wang_color> &colors () const noexcept {
        return m_colors;
      }

      void add_tile (wang_tile tl) {
        m_tiles.emplace_back (std::move (tl));
      }

      [[nodiscard]] const std::vector<wang_tile> &tiles () const noexcept {
        return m_tiles;
      }

    private:
      std::string m_name;
      int m_cell;
      std::vector<wang_color> m_colors;
      std::vector<wang_tile> m_tiles;

  };
}

#endif //NEUTRINO_WANG_SET_HH
