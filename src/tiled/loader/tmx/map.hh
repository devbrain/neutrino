//
// Created by igor on 21/07/2021.
//

#ifndef NEUTRINO_MAP_HH
#define NEUTRINO_MAP_HH

#include <utility>
#include <variant>

#include "component.hh"
#include "layer.hh"
#include "tile_set.hh"
#include "xml.hh"
#include "color.hh"
#include "path_resolver.hh"
#include "tile_layer.hh"
#include "object_layer.hh"
#include "image_layer.hh"
#include "group.hh"

namespace neutrino::tiled::tmx
{

    using layer_t = std::variant<image_layer, tile_layer>;

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


    class map : public component
    {
    public:
        static map parse(const xml_node& node, path_resolver_t resolver);

        /**
         * @brief Map constructor.
         */
        map(std::string version, orientation_t orientation, unsigned width, unsigned height,
            unsigned tilewidth, unsigned tileheight, const colori& bgcolor, render_order_t renderOrder,
            unsigned hexSideLength, stagger_axis_t axis, stagger_index_t index, bool infinite)
                : m_version(std::move(version)), m_orientation(orientation), m_width(width), m_height(height),
                  m_tilewidth(tilewidth), m_tileheight(tileheight), m_bgcolor(bgcolor), m_renderOrder(renderOrder),
                  m_hexSideLength(hexSideLength), m_axis(axis), m_index(index), m_infinite(infinite)
        {
        }

        [[nodiscard]] const std::string& version() const noexcept
        {
            return m_version;
        }

        [[nodiscard]] orientation_t orientation() const noexcept
        {
            return m_orientation;
        }

        [[nodiscard]] unsigned width() const noexcept
        {
            return m_width;
        }

        [[nodiscard]] unsigned height() const noexcept
        {
            return m_height;
        }

        [[nodiscard]] unsigned tile_width() const noexcept
        {
            return m_tilewidth;
        }

        [[nodiscard]] unsigned tile_height() const noexcept
        {
            return m_tileheight;
        }

        [[nodiscard]] colori background_color() const noexcept
        {
            return m_bgcolor;
        }

        [[nodiscard]] render_order_t render_order() const noexcept
        {
            return m_renderOrder;
        }

        [[nodiscard]] unsigned hex_side_length() const noexcept
        {
            return m_hexSideLength;
        }

        [[nodiscard]] stagger_axis_t stagger_axis() const noexcept
        {
            return m_axis;
        }

        [[nodiscard]] stagger_index_t stagger_index() const noexcept
        {
            return m_index;
        }

        [[nodiscard]] bool infinite() const noexcept {
            return m_infinite;
        }

        void add(tile_set tileset)
        {
            m_tilesets.emplace_back(std::move(tileset));
        }


        [[nodiscard]] const std::vector<tile_set>& tile_sets() const noexcept
        {
            return m_tilesets;
        }


        [[nodiscard]] const tile_set* tile_set_from_gid(unsigned gid) const noexcept;

        void add(tile_layer alayer)
        {
            m_layers.emplace_back(std::move(alayer));
        }

        void add(image_layer alayer)
        {
            m_layers.emplace_back(std::move(alayer));
        }

        void add(object_layer alayer)
        {
            m_object_layers.emplace_back(std::move(alayer));
        }

        [[nodiscard]] const std::vector<object_layer>& objects() const noexcept {
            return m_object_layers;
        }


        [[nodiscard]] const std::vector<layer_t>& layers() const noexcept
        {
            return m_layers;
        }
    private:
        static void parse_group(const xml_node& elt, map& result, const group* parent, path_resolver_t resolver);
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

        const bool m_infinite;

        std::vector<tile_set> m_tilesets;
        std::vector<layer_t> m_layers;
        std::vector<object_layer> m_object_layers;
    };
}

#endif

