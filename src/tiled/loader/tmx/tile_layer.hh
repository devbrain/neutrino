//
// Created by igor on 20/07/2021.
//

#ifndef NEUTRINO_TILE_LAYER_HH
#define NEUTRINO_TILE_LAYER_HH

#include "cell.hh"
#include "layer.hh"
#include "group.hh"
#include "reader.hh"
#include <vector>

namespace neutrino::tiled::tmx
{

    class chunk {
    public:
        static chunk parse (const reader& elt, const std::string& encoding, const std::string& compression, int chunk_id=0);

        chunk (int x, int y, int w, int h)
        : m_x(x), m_y(y), m_width(w), m_height(h) {}

        chunk(chunk&&) = default;

        [[nodiscard]] int x() const noexcept {
            return m_x;
        }
        [[nodiscard]] int y() const noexcept {
            return m_y;
        }
        [[nodiscard]] int width() const noexcept {
            return m_width;
        }
        [[nodiscard]] int height() const noexcept {
            return m_height;
        }
        void add(cell acell)
        {
            m_cells.emplace_back(acell);
        }
        [[nodiscard]] const std::vector<cell>& cells() const noexcept {
            return m_cells;
        }
    private:
        int m_x;
        int m_y;
        int m_width;
        int m_height;

        std::vector<cell> m_cells;
    };

    /**
     * @brief A tile layer is a layer with tiles in cells.
     */
    class tile_layer : public layer
    {
    public:
        static tile_layer parse(const reader& elt, const group* parent = nullptr);
        /**
         * @brief TileLayer constructor.
         */
        tile_layer(std::string name, double opacity, bool visible, int id,
                   int offsetx, int offsety, float parallax_x, float parallax_y,
                   colori tint, int width, int height)
                : layer(std::move(name), opacity, visible, id),
                m_offsetx(offsetx),
                m_offsety(offsety),
                m_parallax_x(parallax_x),
                m_parallax_y(parallax_y),
                m_tint(tint),
                m_width(width),
                m_height(height)
        {
        }

        tile_layer(tile_layer&& ) = default;

        void add(cell acell)
        {
            m_cells.emplace_back(acell);
        }

        void add(chunk achunk)
        {
            m_chunks.emplace_back(std::move(achunk));
        }

        [[nodiscard]] const std::vector<cell>& cells() const noexcept {
            return m_cells;
        }

        [[nodiscard]] const std::vector<chunk>& chunks() const noexcept {
            return m_chunks;
        }

        [[nodiscard]] int offset_x() const noexcept {
            return m_offsetx;
        }

        [[nodiscard]] int offset_y() const noexcept {
            return m_offsety;
        }

        [[nodiscard]] colori tint() const noexcept {
            return m_tint;
        }

        [[nodiscard]] float parallax_x() const noexcept {
            return m_parallax_x;
        }

        [[nodiscard]] float parallax_y() const noexcept {
            return m_parallax_y;
        }

        [[nodiscard]] int width() const noexcept {
            return m_width;
        }

        [[nodiscard]] int height() const noexcept {
            return m_height;
        }
    private:
        int m_offsetx;
        int m_offsety;
        float m_parallax_x;
        float m_parallax_y;
        colori m_tint;
        int m_width;
        int m_height;

        std::vector<cell> m_cells;
        std::vector<chunk> m_chunks;
    };
}

#endif

