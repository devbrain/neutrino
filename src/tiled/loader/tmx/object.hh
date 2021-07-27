//
// Created by igor on 20/07/2021.
//

#ifndef NEUTRINO_OBJECT_HH
#define NEUTRINO_OBJECT_HH

#include <string>
#include <utility>
#include <vector>
#include <variant>

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
         * @brief object constructor.
         */
        object(unsigned id, std::string  name, std::string  type,
               const math::point2d& origin, double rotation, bool visible)
                : m_id(id),
                  m_name(std::move(name)),
                  m_type(std::move(type)),
                  m_origin(origin),
                  m_rotation(rotation),
                  m_visible(visible)
        {
        }

        object(const object&) = default;
        object& operator = (const object&) = default;
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
        [[nodiscard]] std::string name() const noexcept {
            return m_name;
        }

        /**
         * @brief Get the type of the object.
         *
         * @return the type of the object.
         */
        [[nodiscard]] std::string type() const noexcept {
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
        [[nodiscard]] math::point2d origin() const noexcept {
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

    private:
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
        tile_object(unsigned id, std::string name, std::string type,
                    math::point2d origin, double rotation, bool visible, unsigned gid, bool hflip, bool vflip, bool dflip)
                : object(id, std::move(name), std::move(type), origin, rotation, visible)
                , m_gid(gid), m_hflip(hflip), m_vflip(vflip), m_dflip(dflip) {}

        tile_object(const tile_object&) = default;
        tile_object& operator = (const tile_object&) = default;

        /**
         * @brief Get the global id of the refering tile (if needed)
         *
         * @return the global id
         */
        [[nodiscard]] unsigned gid() const noexcept {
            return m_gid;
        }

        /**
         * @brief Tell whether the tile object must be flipped horizontally.
         *
         * @returns true if the tile object must be flipped horizontally
         */
        [[nodiscard]] bool is_horizontally_flipped() const noexcept {
            return m_hflip;
        }

        /**
         * @brief Tell whether the tile object must be flipped vertically.
         *
         * @returns true if the tile object must be flipped vertically
         */
        [[nodiscard]] bool is_vertically_flipped() const noexcept {
            return m_vflip;
        }

        /**
         * @brief Tell whether the tile object must be flipped diagonally.
         *
         * @returns true if the tile object must be flipped diagonally
         */
        [[nodiscard]] bool is_diagonally_flipped() const noexcept {
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
        boxed(unsigned id, std::string name, std::string type,
              math::point2d origin, double rotation, bool visible, unsigned width, unsigned height)
                : object(id, std::move(name), std::move(type), origin, rotation, visible), m_width(width), m_height(height)
        {
        }

        boxed(const boxed&) = default;
        boxed& operator = (const boxed&) = default;

        /**
         * @brief Get the width of the box.
         *
         * @returns the width of the box
         */
        [[nodiscard]] unsigned width() const noexcept {
            return m_width;
        }

        /**
         * @brief Get the height of the box.
         *
         * @returns the height of the box
         */
        [[nodiscard]] unsigned height() const noexcept {
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
        rectangle(unsigned id, std::string name, std::string type,
                  math::point2d origin, double rotation, bool visible, unsigned width, unsigned height)
                : boxed(id, std::move(name), std::move(type), origin, rotation, visible, width, height)
        {
        }

        rectangle(const rectangle&) = default;
        rectangle& operator = (const rectangle&) = default;
    };

    /**
     * @brief An ellipse is a geometrical object.
     */
    class ellipse : public boxed {
    public:

        /**
         * @brief Ellipse constructor.
         */
        ellipse(unsigned id, std::string name, std::string type,
                math::point2d origin, double rotation, bool visible, unsigned width, unsigned height)
                : boxed(id, std::move(name), std::move(type), origin, rotation, visible, width, height)
        {
        }

        ellipse(const ellipse&) = default;
        ellipse& operator = (const ellipse&) = default;
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
        chain(unsigned id, std::string name, std::string type, math::point2d origin, double rotation, bool visible)
            : object(id, std::move(name), std::move(type), origin, rotation, visible)
        {
        }

        chain(const chain&) = default;
        chain& operator = (const chain&) = default;

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
        polyline(unsigned id, std::string name, std::string type, math::point2d origin, double rotation, bool visible)
                : chain(id, std::move(name), std::move(type), origin, rotation, visible)
        {
        }

        polyline(const polyline&) = default;
        polyline& operator = (const polyline&) = default;
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
        polygon(unsigned id, std::string name, std::string type, math::point2d origin, double rotation,
                bool visible)
                : chain(id, std::move(name), std::move(type), origin, rotation, visible)
        {
        }

        polygon(const polygon&) = default;
        polygon& operator = (const polygon&) = default;
    };

    using object_t = std::variant<rectangle, ellipse, polygon, polyline, tile>;
}

#endif
