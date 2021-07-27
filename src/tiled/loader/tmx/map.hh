//
// Created by igor on 21/07/2021.
//

#ifndef NEUTRINO_MAP_HH
#define NEUTRINO_MAP_HH

#include <variant>

#include "component.hh"
#include "layer.hh"
#include "tile_set.hh"
#include "xml.hh"
#include "color.hh"
#include "path_resolver.hh"
#include "tile_layer.hh"
#include "object_layer.hh"

namespace neutrino::tiled::tmx
{

    using layer_t = std::variant<object_layer, tile_layer>;

    /**
   * @brief the orientation of the map.
   */
    enum class orientation_t
    {
        UNKNOWN,    /**< Unknown orientation */
        ORTHOGONAL, /**< Orthogonal orientation */
        ISOMETRIC,  /**< Isometric orientation */
        STAGGERED,  /**< Staggered orientation */
        HEXAGONAL,  /**< Hexagonal orientation */
    };

    /**
     * @brief Stagger index of the hexagonal map.
     */
    enum class stagger_index_t
    {
        ODD,
        EVEN,
    };

    /**
     * @brief Stagger axis of the hexagonal map.
     */
    enum class stagger_axis_t
    {
        X,
        Y,
    };

    /**
     * @brief the render order of the tiles.
     */
    enum class render_order_t
    {
        RIGHT_DOWN, /**< Right down order */
        RIGHT_UP,   /**< Right up order */
        LEFT_DOWN,  /**< Left down order */
        LEFT_UP,    /**< Left up order */
    };

    /**
     * @brief A map is a set of tilesets and a set of different layers.
     *
     * Tilesets describe what to draw while layers describe how to draw things.
     */
    class map : public component
    {
    public:
        static map parse(const xml_node& node, path_resolver_t resolver);

        /**
         * @brief Map constructor.
         */
        map(const std::string version, orientation_t orientation, unsigned width, unsigned height,
            unsigned tilewidth, unsigned tileheight, const colori& bgcolor, render_order_t renderOrder,
            unsigned hexSideLength, stagger_axis_t axis, stagger_index_t index, unsigned nextObjectId)
                : m_version(version), m_orientation(orientation), m_width(width), m_height(height),
                  m_tilewidth(tilewidth), m_tileheight(tileheight), m_bgcolor(bgcolor), m_renderOrder(renderOrder),
                  m_hexSideLength(hexSideLength), m_axis(axis), m_index(index), m_nextObjectId(nextObjectId)
        {
        }

        /**
         * @name Properties
         * @{
         */
        /**
         * @brief Get the version of the TMX format.
         *
         * @returns the version of the TMX format (generally "1.0")
         */
        [[nodiscard]] const std::string& version() const noexcept
        {
            return m_version;
        }

        /**
         * @brief Get the orientation of the map.
         *
         * @returns the orientation of the map
         */
        [[nodiscard]] orientation_t orientation() const noexcept
        {
            return m_orientation;
        }

        /**
         * @brief Get the width of the map.
         *
         * @returns the width of the map (in number of tiles)
         */
        [[nodiscard]] unsigned width() const noexcept
        {
            return m_width;
        }

        /**
         * @brief Get the height of the map.
         *
         * @returns the height of the map (in number of tiles)
         */
        [[nodiscard]] unsigned height() const noexcept
        {
            return m_height;
        }

        /**
         * @brief Get the width of tiles.
         *
         * @returns the width of tiles (in pixels)
         */
        [[nodiscard]] unsigned tile_width() const noexcept
        {
            return m_tilewidth;
        }

        /**
         * @brief Get the height of tiles.
         *
         * @returns the height of tiles (in pixels)
         */
        [[nodiscard]] unsigned tile_height() const noexcept
        {
            return m_tileheight;
        }

        /**
         * @brief Get the background color.
         *
         * @returns the background color
         */
        [[nodiscard]] colori background_color() const noexcept
        {
            return m_bgcolor;
        }

        /**
         * @brief Get the render order.
         *
         * @returns the render order
         */
        [[nodiscard]] render_order_t render_order() const noexcept
        {
            return m_renderOrder;
        }

        /**
         * @brief Get the hexagonal side length.
         *
         * Only relevant for hexagonal maps.
         *
         * @returns the hexagonal side length
         */
        [[nodiscard]] unsigned hex_side_length() const noexcept
        {
            return m_hexSideLength;
        }

        /**
         * @brief Get the staggered axis.
         *
         * Only relevant for staggered and hexagonal maps.
         *
         * @returns the staggered axis
         */
        [[nodiscard]] stagger_axis_t stagger_axis() const noexcept
        {
            return m_axis;
        }

        /**
         * @brief Get the staggered index.
         *
         * Only relevant for staggered and hexagonal maps.
         *
         * @returns the staggered index
         */
        [[nodiscard]] stagger_index_t stagger_index() const noexcept
        {
            return m_index;
        }

        /**
         * @brief Get the next object id.
         *
         * @returns the next object id.
         */
        [[nodiscard]] unsigned next_object_id() const noexcept
        {
            return m_nextObjectId;
        }

        /** @} */

        /**
         * @brief A tileset range.
         */


        /**
         * @name Tileset handling
         * @{
         */
        /**
         * @brief Add a tileset.
         *
         * @param tileset the tileset
         */
        void add_tile_set(tile_set&& tileset)
        {
            m_tilesets.emplace_back(std::move(tileset));
        }

        /**
         * @brief Get the tilesets.
         *
         * @return a tileset range
         */
        [[nodiscard]] const std::vector<tile_set>& tile_sets() const noexcept
        {
            return m_tilesets;
        }

        /**
         * @brief Get the tileset corresponding to a global id.
         *
         * @param gid a global id
         * @returns the corresponding tileset
         */
        [[nodiscard]] const tile_set* tile_set_from_gid(unsigned gid) const noexcept;
        /** @} */

        /**
         * @brief A layer range.
         */

        /**
         * @name Layer handling
         * @{
         */
        /**
         * @brief Add a layer.
         *
         * @param layer the layer
         */
        void add_layer(tile_layer alayer)
        {
            m_layers.emplace_back(std::move(alayer));
        }

        void add_layer(object_layer alayer)
        {
            m_layers.emplace_back(std::move(alayer));
        }

        /**
         * @brief Get the layers.
         *
         * @returns a layer range
         */
        [[nodiscard]] const std::vector<layer_t>& get_layers() const noexcept
        {
            return m_layers;
        }

        /**
         * @brief Visit the layers with a visitor.
         *
         * @param visitor the visitor
         */
        /*
       void visitLayers(LayerVisitor& visitor) const {
           for (auto layer : getLayers()) {
               layer->accept(*this, visitor);
           }
       }
         */
        /** @} */


    private:
        const std::string m_version;

        const orientation_t m_orientation;

        const unsigned m_width;
        const unsigned m_height;

        const unsigned m_tilewidth;
        const unsigned m_tileheight;

        const colori m_bgcolor;

        const render_order_t m_renderOrder;

        const unsigned m_hexSideLength;
        const stagger_axis_t m_axis;
        const stagger_index_t m_index;

        const unsigned m_nextObjectId;

        std::vector<tile_set> m_tilesets;
        std::vector<layer_t> m_layers;
    };
}

#endif

