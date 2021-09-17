//
// Created by igor on 20/07/2021.
//

#ifndef NEUTRINO_TILE_SET_HH
#define NEUTRINO_TILE_SET_HH

#include "component.hh"
#include "image.hh"
#include "terrain.hh"
#include "tile.hh"
#include "reader.hh"
#include "neutrino/tiled/loader/path_resolver.hh"
#include "wang_set.hh"
#include <neutrino/math/rect.hh>
#include <utility>
#include <vector>
#include <memory>
#include <optional>

namespace neutrino::tiled::tmx {
  /**
   * @brief A tileset is a set of tiles in a single file (image or TSX file).
   */
  class tile_set : public component {
    public:
      class grid {
        public:
          static grid parse (const reader& elt);
        public:
          grid (bool ortho, unsigned w, unsigned h)
              : m_orthogonal (ortho),
                m_width (w),
                m_height (h) {
          }

          [[nodiscard]] bool is_orthogonal () const noexcept {
            return m_orthogonal;
          }

          [[nodiscard]] unsigned width () const noexcept {
            return m_width;
          }

          [[nodiscard]] unsigned height () const noexcept {
            return m_height;
          }

        private:
          bool m_orthogonal;
          unsigned m_width;
          unsigned m_height;
      };

    public:
      static tile_set parse (const reader& elt, const path_resolver_t& resolver);

      tile_set (unsigned firstgid, std::string name, unsigned tilewidth, unsigned tileheight,
                unsigned spacing, unsigned margin, unsigned tilecount, unsigned columns)
          : m_firstgid (firstgid), m_name (std::move (name)), m_tilewidth (tilewidth), m_tileheight (tileheight),
            m_spacing (spacing), m_margin (margin), m_tilecount (tilecount), m_columns (columns),
            m_x (0), m_y (0), m_image (nullptr) {
      }

      [[nodiscard]] unsigned first_gid () const noexcept {
        return m_firstgid;
      }

      [[nodiscard]] const std::string& name () const noexcept {
        return m_name;
      }

      [[nodiscard]] unsigned tile_width () const noexcept {
        return m_tilewidth;
      }

      [[nodiscard]] unsigned tile_height () const noexcept {
        return m_tileheight;
      }

      [[nodiscard]] unsigned spacing () const noexcept {
        return m_spacing;
      }

      [[nodiscard]] unsigned margin () const noexcept {
        return m_margin;
      }

      [[nodiscard]] unsigned tile_count () const noexcept {
        return m_tilecount;
      }

      void offset (int x, int y) noexcept {
        m_x = x;
        m_y = y;
      }

      [[nodiscard]] int offset_x () const noexcept {
        return m_x;
      }

      [[nodiscard]] int offset_y () const noexcept {
        return m_y;
      }

      void set_image (std::unique_ptr<image> aimage) {
        m_image = std::move (aimage);
      }

      [[nodiscard]] bool has_image () const noexcept {
        return m_image != nullptr;
      }

      [[nodiscard]] const image* get_image () const noexcept {
        return m_image.get ();
      }

      [[nodiscard]] image* get_image () noexcept {
        return m_image.get ();
      }

      void add_terrain (terrain aterrain) {
        m_terrains.emplace_back (std::move (aterrain));
      }

      [[nodiscard]] const std::vector<terrain>& get_terrains () const noexcept {
        return m_terrains;
      }

      typedef std::vector<tile>::const_iterator const_iterator;

      void add_tile (tile atile) {
        m_tiles.emplace_back (std::move (atile));
      }

      [[nodiscard]] const_iterator begin () const noexcept {
        return m_tiles.cbegin ();
      }

      [[nodiscard]] const_iterator end () const noexcept {
        return m_tiles.cend ();
      }

      [[nodiscard]] const tile* get_tile (unsigned id) const noexcept;

      [[nodiscard]] math::rect get_coords (unsigned id, math::dimension_t size) const noexcept;

      [[nodiscard]] unsigned columns () const noexcept {
        return m_columns;
      }

      [[nodiscard]] const std::vector<wang_set>& wang_sets () const noexcept {
        return m_wang_sets;
      }

      void add_wang_set (wang_set ws) {
        m_wang_sets.emplace_back (std::move (ws));
      }

      void add_grid (grid g) {
        m_grid = g;
      }

      [[nodiscard]] std::optional<grid> grid_info () const noexcept {
        return m_grid;
      }

    private:
      static tile_set parse_inner (unsigned first_gid, const reader& elt);
      static tile_set parse_from_file (unsigned first_gid, const std::string& source, const path_resolver_t& resolver);
    private:
      const unsigned m_firstgid;
      const std::string m_name;
      const unsigned m_tilewidth;
      const unsigned m_tileheight;
      const unsigned m_spacing;
      const unsigned m_margin;
      const unsigned m_tilecount;
      const unsigned m_columns;

      int m_x;
      int m_y;

      std::unique_ptr<image> m_image;
      std::vector<terrain> m_terrains;
      std::vector<tile> m_tiles;
      std::vector<wang_set> m_wang_sets;
      std::optional<grid> m_grid;
  };

}

#endif
