//
// Created by igor on 20/07/2021.
//

#ifndef NEUTRINO_TILE_LAYER_HH
#define NEUTRINO_TILE_LAYER_HH

#include "cell.hh"
#include "layer.hh"
#include "xml.hh"
#include <vector>

namespace neutrino::tiled::tmx
{
    /**
  * @brief A tile layer is a layer with tiles in cells.
  */
    class tile_layer : public layer
    {
    public:
        static tile_layer parse(const xml_node& elt);
        /**
         * @brief TileLayer constructor.
         */
        tile_layer(std::string name, double opacity, bool visible)
                : layer(std::move(name), opacity, visible)
        {
        }

        tile_layer(tile_layer&& ) = default;

        /**
         * @brief Add a cell to the layer.
         *
         * @param cell the cell
         */
        void add(cell acell)
        {
            m_cells.emplace_back(acell);
        }

        /**
         * @brief A cell iterator.
         */
        typedef typename std::vector<cell>::const_iterator const_iterator;

        [[nodiscard]] const std::vector<cell>& cells() const noexcept {
            return m_cells;
        }

        /**
         * @brief Get the begin iterator on the cells.
         *
         * @return the begin iterator
         */
        [[nodiscard]] const_iterator begin() const noexcept
        {
            return m_cells.cbegin();
        }

        /**
         * @brief Get the end iterator on the cells.
         *
         * @return the end iterator
         */
        [[nodiscard]] const_iterator end() const noexcept
        {
            return m_cells.cend();
        }
    private:
        std::vector<cell> m_cells;
    };
}

#endif

