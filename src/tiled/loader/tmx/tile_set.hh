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
#include "path_resolver.hh"
#include <neutrino/math/rect.hh>
#include <vector>
#include <memory>

namespace neutrino::tiled::tmx {
    /**
  * @brief A tileset is a set of tiles in a single file (image or TSX file).
  */
    class tile_set : public component {
    public:
        static tile_set parse(const reader& elt, const path_resolver_t& resolver);
        /**
         * @brief TileSet constructor.
         */
        tile_set(unsigned firstgid, const std::string& name, unsigned tilewidth, unsigned tileheight,
                 unsigned spacing, unsigned margin, unsigned tilecount, unsigned columns)
                : m_firstgid(firstgid), m_name(name), m_tilewidth(tilewidth), m_tileheight(tileheight),
                  m_spacing(spacing), m_margin(margin), m_tilecount(tilecount), m_columns(columns),
                  m_x(0), m_y(0), m_image(nullptr)
        {
        }

        /**
         * @brief Get the first global id of this tileset.
         *
         * @return the first global id of the tileset
         */
        [[nodiscard]] unsigned first_gid() const noexcept {
            return m_firstgid;
        }

        /**
         * @brief Get the name of the tileset.
         *
         * @return the name of the tileset
         */
        [[nodiscard]] const std::string& name() const noexcept {
            return m_name;
        }

        /**
         * @brief Get the width of the tiles.
         *
         * @return the width of the tiles
         */
        [[nodiscard]] unsigned tile_width() const noexcept {
            return m_tilewidth;
        }

        /**
         * @brief Get the height of the tiles.
         *
         * @return the height of the tiles
         */
        [[nodiscard]] unsigned tile_height() const noexcept {
            return m_tileheight;
        }

        /**
         * @brief Get the spacing between tiles.
         *
         * @return the spacing between tiles (in pixels)
         */
        [[nodiscard]] unsigned spacing() const noexcept {
            return m_spacing;
        }

        /**
         * @brief Get the margin around tiles.
         *
         * @returns the margin around tiles (in pixels)
         */
        [[nodiscard]] unsigned margin() const noexcept {
            return m_margin;
        }

        /**
         * @brief Get the tile count
         *
         * @returns the tile count
         */
        [[nodiscard]] unsigned tile_count() const noexcept {
            return m_tilecount;
        }

        /**
         * @brief Set the offset of the tileset.
         *
         * @param x the x coordinate of the offset
         * @param y the y coordinate of the offset
         */
        void offset(int x, int y) noexcept {
            m_x = x;
            m_y = y;
        }

        /**
         * @brief Get the x offset of the tileset.
         *
         * @returns the x offset of the tileset (in pixels)
         */
        [[nodiscard]] int offset_x() const noexcept {
            return m_x;
        }

        /**
         * @brief Get the y offset of the tileset.
         *
         * @returns the y offset of the tileset (in pixels)
         */
        [[nodiscard]] int offset_y() const noexcept {
            return m_y;
        }
        /** @} */

        /**
         * @name Image handing
         * @{
         */
        /**
         * @brief Set an image associated to the tileset.
         *
         * @param image the image associated to the tileset
         */
        void set_image(std::unique_ptr<image> aimage) {
            m_image = std::move(aimage);
        }

        /**
         * @brief Tell whether the tileset has an image.
         *
         * @returns true if the tileset has an image
         */
        [[nodiscard]] bool has_image() const noexcept {
            return m_image != nullptr;
        }

        /**
         * @brief Get the image associated to the tileset.
         *
         * @returns the image associated to the tileset
         */
        [[nodiscard]] const image *get_image() const noexcept {
            return m_image.get();
        }
        /** @} */




        /**
         * @name Terrain handling
         * @{
         */
        /**
         * @brief Add terrain information to the tileset.
         *
         * @param terrain the terrain information
         */
        void add_terrain(terrain aterrain) {
            m_terrains.emplace_back(std::move(aterrain));
        }

        /**
         * @brief Get the terrains.
         *
         * @returns a terrain range
         */
        [[nodiscard]] const std::vector<terrain>& get_terrains() const noexcept {
            return m_terrains;
        }
        /** @} */

        /**
         * @brief A tile iterator.
         */
        typedef std::vector<tile>::const_iterator const_iterator;

        /**
         * @name Tile handling
         * @{
         */
        /**
         * @brief Add a tile to the tileset.
         *
         * @param tile the tile
         */
        void add_tile(tile atile) {
            m_tiles.emplace_back(std::move(atile));
        }

        /**
         * @brief Get the begin iterator on the tiles.
         *
         * @return the begin iterator on the tiles
         */
        [[nodiscard]] const_iterator begin() const noexcept {
            return m_tiles.cbegin();
        }

        /**
         * @brief Get the end iterator on the tiles.
         *
         * @return the end iterator on the tiles
         */
        [[nodiscard]] const_iterator end() const noexcept {
            return m_tiles.cend();
        }

        /**
         * @brief Get the tile corresponding to an id.
         *
         * @param id the id of the tile
         * @returns the tile
         */
        [[nodiscard]] const tile *get_tile(unsigned id) const noexcept;

        /**
         * @brief Get the coordinates of a tile corresponding to an id.
         *
         * @param id the id of the tile
         * @param size the size of the image corresponding to the tile
         * @returns the coordinates in the form of a rectangle
         */
        [[nodiscard]] math::rect get_coords(unsigned id, math::dimension_t size) const noexcept;
        /** @} */

        [[nodiscard]] unsigned columns() const noexcept {
            return m_columns;
        }
    private:
        static tile_set parse_inner(unsigned first_gid, const reader& elt);
        static tile_set parse_from_file(unsigned first_gid, const std::string& source, const path_resolver_t& resolver);
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
    };

}

#endif
