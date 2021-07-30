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
#include "color.hh"
#include <neutrino/math/rect.hh>
#include <neutrino/math/point.hh>

namespace neutrino::tiled::tmx
{
    /**
   * @brief An object is a geometrical object.
   *
   * There four kinds of geometrical objects: rectangles, ellipses, polylines
   * and polygons.
   */

    struct object_attribs
    {
        object_attribs(unsigned id, std::string name, std::string type,
                       const math::point2d& origin, int width, int height, double rotation, bool visible,
                       unsigned gid, bool hflip, bool vflip, bool dflip)
                : m_id(id),
                  m_name(std::move(name)),
                  m_type(std::move(type)),
                  m_origin(origin),
                  m_width(width),
                  m_height(height),
                  m_rotation(rotation),
                  m_visible(visible),
                  m_gid(gid),
                  m_hflip(hflip),
                  m_vflip(vflip),
                  m_dflip(dflip)
        {}

        unsigned m_id;
        std::string m_name;
        std::string m_type;
        math::point2d m_origin;
        int m_width;
        int m_height;
        double m_rotation;
        bool m_visible;
        unsigned m_gid;
        bool m_hflip;
        bool m_vflip;
        bool m_dflip;
    };

    class object : public component
    {
    public:

        /**
         * @brief object constructor.
         */
        object(object_attribs atts)
                : m_id(atts.m_id),
                  m_name(std::move(atts.m_name)),
                  m_type(std::move(atts.m_type)),
                  m_origin(atts.m_origin),
                  m_width(atts.m_width),
                  m_height(atts.m_height),
                  m_rotation(atts.m_rotation),
                  m_visible(atts.m_visible),
                  m_gid(atts.m_gid),
                  m_hflip(atts.m_hflip),
                  m_vflip(atts.m_vflip),
                  m_dflip(atts.m_dflip)
        {}

        object(const object&) = default;

        /**
         * @brief Get the id of the object.
         *
         * @returns the id of the object
         */
        [[nodiscard]] unsigned id() const noexcept
        {
            return m_id;
        }

        /**
         * @brief Get the name of the object.
         *
         * @return the name of the object
         */
        [[nodiscard]] std::string name() const noexcept
        {
            return m_name;
        }

        /**
         * @brief Get the type of the object.
         *
         * @return the type of the object.
         */
        [[nodiscard]] std::string type() const noexcept
        {
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
        [[nodiscard]] math::point2d origin() const noexcept
        {
            return m_origin;
        }

        [[nodiscard]] int width() const noexcept
        {
            return m_width;
        }

        [[nodiscard]] int height() const noexcept
        {
            return m_height;
        }

        /**
         * @brief Get the rotation of the object.
         *
         * @return the angle of rotation in degrees clockwise
         */
        [[nodiscard]] double rotation() const noexcept
        {
            return m_rotation;
        }

        /**
         * @brief Tell whether this object is visible.
         *
         * @returns true if the object is visible
         */
        [[nodiscard]] bool visible() const noexcept
        {
            return m_visible;
        }
        /**
        * @brief Get the global id of the refering tile (if needed)
        *
        * @return the global id
        */
        [[nodiscard]] unsigned gid() const noexcept
        {
            return m_gid;
        }

        /**
         * @brief Tell whether the tile object must be flipped horizontally.
         *
         * @returns true if the tile object must be flipped horizontally
         */
        [[nodiscard]] bool is_horizontally_flipped() const noexcept
        {
            return m_hflip;
        }

        /**
         * @brief Tell whether the tile object must be flipped vertically.
         *
         * @returns true if the tile object must be flipped vertically
         */
        [[nodiscard]] bool is_vertically_flipped() const noexcept
        {
            return m_vflip;
        }

        /**
         * @brief Tell whether the tile object must be flipped diagonally.
         *
         * @returns true if the tile object must be flipped diagonally
         */
        [[nodiscard]] bool is_diagonally_flipped() const noexcept
        {
            return m_dflip;
        }
    private:
        unsigned m_id;
        std::string m_name;
        std::string m_type;
        math::point2d m_origin;
        int m_width;
        int m_height;
        double m_rotation;
        bool m_visible;
        unsigned m_gid;
        bool m_hflip;
        bool m_vflip;
        bool m_dflip;
    };

    class ellipse : public object
    {
    public:
        /**
         * @brief Tileobject constructor.
         */
        explicit ellipse(object_attribs a)
                : object(std::move(a))
        {}

        ellipse(const ellipse&) = default;
    };

    class point : public object
    {
    public:
        /**
         * @brief Tileobject constructor.
         */
        explicit point(object_attribs a)
                : object(std::move(a))
        {}

        point(const point&) = default;
    };

    /**
     * @brief A chain is a base class for lines with multiple points.
     *
     * There are two kinds of chain: polylines and polygons.
     */
    class chain : public object
    {
    public:
        /**
         * @brief Chain constructor.
         */
        explicit chain(object_attribs a)
                : object(std::move(a))
        {
        }

        chain(const chain&) = default;

        /**
         * @brief Set the points of the lines.
         *
         * @param points the points
         */
        void points(std::vector<math::point2d> points)
        {
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
        [[nodiscard]] const_iterator begin() const noexcept
        {
            return m_points.cbegin();
        }

        /**
         * @brief Get the end iterator on the points.
         *
         * @returns the end iterator
         */
        [[nodiscard]] const_iterator end() const noexcept
        {
            return m_points.cend();
        }

        [[nodiscard]] const std::vector<math::point2d>& points () const noexcept {
            return m_points;
        }
    private:
        std::vector<math::point2d> m_points;
    };

    /**
     * @brief A polyline is an open set of lines.
     */
    class polyline : public chain
    {
    public:
        /**
         * @brief Polyline constructor.
         */
        explicit polyline(object_attribs a)
                : chain(std::move(a))
        {
        }

        polyline(const polyline&) = default;

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
        explicit polygon(object_attribs a)
                : chain(std::move(a))
        {
        }

        polygon(const polygon&) = default;

    };

    class text : public object
    {
    public:
        enum class halign_t
        {
            LEFT,
            CENTER,
            RIGHT,
            JUSTIFY
        };
        enum class valign_t
        {
            TOP,
            CENTER,
            BOTTOM
        };

        explicit text(object_attribs a)
        : object(a) {}

        void parse(const xml_node& elt);

        text(object_attribs a, std::string font_family, int pixel_size, bool wrap,
             colori color, bool bold, bool italic, bool underline, bool strike,
             bool kerning, halign_t halign, valign_t valign, const std::string& data)
                : object(std::move(a)),
                  m_font_family(std::move(font_family)),
                  m_pixel_size(pixel_size),
                  m_wrap(wrap),
                  m_color(color),
                  m_bold(bold),
                  m_italic(italic),
                  m_underline(underline),
                  m_strike(strike),
                  m_kerning(kerning),
                  m_halign(halign),
                  m_valign(valign),
                  m_data(data)
        {}

        [[nodiscard]] std::string font_family() const noexcept
        {
            return m_font_family;
        }

        [[nodiscard]] int pixel_size() const noexcept
        {
            return m_pixel_size;
        }
        [[nodiscard]] bool wrap() const noexcept
        {
            return m_wrap;
        }
        [[nodiscard]] colori color() const noexcept
        {
            return m_color;
        }
        [[nodiscard]] bool bold() const noexcept
        {
            return m_bold;
        }
        [[nodiscard]] bool italic() const noexcept
        {
            return m_italic;
        }
        [[nodiscard]] bool underline() const noexcept
        {
            return m_underline;
        }
        [[nodiscard]] bool strike() const noexcept
        {
            return m_strike;
        }
        [[nodiscard]] bool kerning() const noexcept
        {
            return m_kerning;
        }
        [[nodiscard]] halign_t halign() const noexcept
        {
            return m_halign;
        }
        [[nodiscard]] valign_t valign() const noexcept
        {
            return m_valign;
        }
        [[nodiscard]] const std::string& data() const noexcept
        {
            return m_data;
        }
    private:
        std::string m_font_family;
        int m_pixel_size;
        bool m_wrap;
        colori m_color;
        bool m_bold;
        bool m_italic;
        bool m_underline;
        bool m_strike;
        bool m_kerning;
        halign_t m_halign;
        valign_t m_valign;
        std::string m_data;
    };

    using object_t = std::variant<object, point, ellipse, polygon, polyline, text>;

    object_t parse_object(const xml_node& elt);
}

#endif
