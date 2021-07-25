//
// Created by igor on 20/07/2021.
//

#ifndef NEUTRINO_OBJECT_HH
#define NEUTRINO_OBJECT_HH

#include <string>
#include <vector>

#include "component.hh"
#include <neutrino/math/rect.hh>
#include <neutrino/math/point.hh>

namespace neutrino::tiled::tmx {
    /**
   * @brief An object is a geometrical object.
   *
   * There four kinds of geometrical objects: rectangles, ellipses, polylines
   * and polygons.
   */
    class object : public component {
    public:
        /**
         * @brief A kind of geometrical object.
         */
        enum Kind {
            RECTANGLE,  /**< Rectangle */
            ELLIPSE,    /**< Ellipse */
            POLYLINE,   /**< Polyline (open line) */
            POLYGON,    /**< Polygon (closed line) */
            TILE,       /**< Tile (image) */
        };

        /**
         * @brief object constructor.
         */
        object(const Kind kind, unsigned id, const std::string& name, const std::string& type,
               const math::point2d& origin, double rotation, bool visible)
                : m_kind(kind), m_id(id), m_name(name), m_type(type), m_origin(origin), m_rotation(rotation), m_visible(visible)
        {
        }


        /**
         * @brief Get the kind of the object.
         *
         * This may be useful to do some casting.
         *
         * @return the kind of the object
         */
        [[nodiscard]] Kind kind() const noexcept {
            return m_kind;
        }

        /**
         * @brief Get the id of the object.
         *
         * @returns the id of the object
         */
        [[nodiscard]] unsigned id() const noexcept {
            return m_id;
        }

        /**
         * @brief Get the name of the object.
         *
         * @return the name of the object
         */
        [[nodiscard]] const std::string& name() const noexcept {
            return m_name;
        }

        /**
         * @brief Get the type of the object.
         *
         * @return the type of the object.
         */
        [[nodiscard]] const std::string& type() const noexcept {
            return m_type;
        }


        /**
         * @brief Get the origin.
         *
         * The origin is the base point of the object on the map. All other
         * coordinates are computed from this point.
         *
         * @return the origin
         */
        [[nodiscard]] const math::point2d& origin() const noexcept {
            return m_origin;
        }

        /**
         * @brief Get the rotation of the object.
         *
         * @return the angle of rotation in degrees clockwise
         */
        [[nodiscard]] double rotation() const noexcept {
            return m_rotation;
        }

        /**
         * @brief Tell whether this object is visible.
         *
         * @returns true if the object is visible
         */
        [[nodiscard]] bool visible() const noexcept {
            return m_visible;
        }

        /**
         * @brief Tell whether this object is a rectangle.
         *
         * @returns true if the object is a rectangle
         */
        [[nodiscard]] bool is_rectangle() const noexcept {
            return m_kind == RECTANGLE;
        }

        /**
         * @brief Tell whether this object is an ellipse.
         *
         * @returns true if the object is an ellipse
         */
        [[nodiscard]] bool is_ellipse() const noexcept {
            return m_kind == ELLIPSE;
        }

        /**
         * @brief Tell whether this object is a chain (polygon or polyline).
         *
         * @returns true if the object is a chain
         */
        [[nodiscard]] bool is_chain() const noexcept {
            return m_kind == POLYLINE || m_kind == POLYGON;
        }

        /**
         * @brief Tell whether this object is a polyline.
         *
         * @returns true if the object is a polyline
         */
        [[nodiscard]] bool is_polyline() const noexcept {
            return m_kind == POLYLINE;
        }

        /**
         * @brief Tell whether this object is a polygon.
         *
         * @returns true if the object is a polygon
         */
        [[nodiscard]] bool is_polygon() const noexcept {
            return m_kind == POLYGON;
        }

        /**
         * @brief Tell whether this object is a tile.
         *
         * @returns true if the object is a tile
         */
        [[nodiscard]] bool is_tile() const noexcept {
            return m_kind == TILE;
        }

    private:
        const Kind m_kind;
        const unsigned m_id;
        const std::string m_name;
        const std::string m_type;
        const math::point2d m_origin;
        const double m_rotation;
        const bool m_visible;
    };

    /**
     * @brief A tile object is an image put in the map.
     */
    class tile_object : public object {
    public:
        /**
         * @brief Tileobject constructor.
         */
        tile_object(unsigned id, const std::string& name, const std::string& type,
                    const math::point2d& origin, double rotation, bool visible, unsigned gid, bool hflip, bool vflip, bool dflip)
                : object(TILE, id, name, type, origin, rotation, visible)
                , m_gid(gid), m_hflip(hflip), m_vflip(vflip), m_dflip(dflip)
        {
        }

        /**
         * @brief Get the global id of the refering tile (if needed)
         *
         * @return the global id
         */
        unsigned gid() const noexcept {
            return m_gid;
        }

        /**
         * @brief Tell whether the tile object must be flipped horizontally.
         *
         * @returns true if the tile object must be flipped horizontally
         */
        bool is_horizontally_flipped() const noexcept {
            return m_hflip;
        }

        /**
         * @brief Tell whether the tile object must be flipped vertically.
         *
         * @returns true if the tile object must be flipped vertically
         */
        bool is_vertically_flipped() const noexcept {
            return m_vflip;
        }

        /**
         * @brief Tell whether the tile object must be flipped diagonally.
         *
         * @returns true if the tile object must be flipped diagonally
         */
        bool is_diagonally_flipped() const noexcept {
            return m_dflip;
        }

    private:
        const unsigned m_gid;
        bool m_hflip;
        bool m_vflip;
        bool m_dflip;
    };

    /**
     * @brief A boxed object is a geometrical object that is defined by a box.
     *
     * There are two kinds of boxed objects: rectangles and ellipses.
     */
    class boxed : public object {
    public:

        /**
         * @brief Boxed constructor.
         */
        boxed(Kind kind, unsigned id, const std::string& name, const std::string& type,
              const math::point2d& origin, double rotation, bool visible, unsigned width, unsigned height)
                : object(kind, id, name, type, origin, rotation, visible), m_width(width), m_height(height)
        {
        }

        /**
         * @brief Get the width of the box.
         *
         * @returns the width of the box
         */
        unsigned width() const noexcept {
            return m_width;
        }

        /**
         * @brief Get the height of the box.
         *
         * @returns the height of the box
         */
        unsigned height() const noexcept {
            return m_height;
        }

    private:
        const unsigned m_width;
        const unsigned m_height;
    };


    /**
     * @brief A rectangle is a geometrical object.
     */
    class rectangle : public boxed {
    public:

        /**
         * @brief Rectangle constructor.
         */
        rectangle(unsigned id, const std::string& name, const std::string& type,
                  const math::point2d& origin, double rotation, bool visible, unsigned width, unsigned height)
                : boxed(RECTANGLE, id, name, type, origin, rotation, visible, width, height)
        {
        }
    };

    /**
     * @brief An ellipse is a geometrical object.
     */
    class ellipse : public boxed {
    public:

        /**
         * @brief Ellipse constructor.
         */
        ellipse(unsigned id, const std::string& name, const std::string& type,
                const math::point2d& origin, double rotation, bool visible, unsigned width, unsigned height)
                : boxed(ELLIPSE, id, name, type, origin, rotation, visible, width, height)
        {
        }
    };

    /**
     * @brief A chain is a base class for lines with multiple points.
     *
     * There are two kinds of chain: polylines and polygons.
     */
    class chain : public object {
    public:
        /**
         * @brief Chain constructor.
         */
        chain(const Kind kind, unsigned id, const std::string& name, const std::string& type, const math::point2d& origin, double rotation, bool visible)
                : object(kind, id, name, type, origin, rotation, visible)
        {
        }

        /**
         * @brief Set the points of the lines.
         *
         * @param points the points
         */
        void points(std::vector<math::point2d> points) {
            m_points = std::move(points);
        }

        /**
         * @brief A point iterator.
         */
        typedef typename std::vector<math::point2d>::const_iterator const_iterator;

        /**
         * @brief Get the begin iterator on the points.
         *
         * @returns the begin iterator
         */
        [[nodiscard]] const_iterator begin() const noexcept {
            return m_points.cbegin();
        }

        /**
         * @brief Get the end iterator on the points.
         *
         * @returns the end iterator
         */
        [[nodiscard]] const_iterator end() const noexcept {
            return m_points.cend();
        }

    private:
        std::vector<math::point2d> m_points;
    };

    /**
     * @brief A polyline is an open set of lines.
     */
    class polyline : public chain {
    public:
        /**
         * @brief Polyline constructor.
         */
        polyline(unsigned id, const std::string& name, const std::string& type, const math::point2d& origin, double rotation, bool visible)
                : chain(POLYLINE, id, name, type, origin, rotation, visible)
        {
        }
    };

    /**
     * @brief A polygon is a closed set of lines.
     */
    class polygon : public chain
    {
    public:
        /**
         * @brief Polygon constructor.
         */
        polygon(unsigned id, const std::string& name, const std::string& type, const math::point2d& origin, double rotation,
                bool visible)
                : chain(POLYLINE, id, name, type, origin, rotation, visible)
        {
        }
    };
}

#endif
