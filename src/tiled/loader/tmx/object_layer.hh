//
// Created by igor on 20/07/2021.
//

#ifndef NEUTRINO_OBJECT_LAYER_HH
#define NEUTRINO_OBJECT_LAYER_HH

#include "object.hh"
#include "layer.hh"
#include "color.hh"
#include "group.hh"
#include <memory>
#include <utility>

namespace neutrino::tiled::tmx {
    /**
   * @brief the order in which the objects should be drawn.
   */
    enum class draw_order_t {
        TOP_DOWN, /**< Top-down order (default) */
        INDEX,    /**< Index order */
    };

    /**
     * @brief An object layer is a layers composed of objects.
     *
     * An object layer is called object group in the TMX format.
     */
    class object_layer : public layer {
    public:
        using objects_vec_t = std::vector<object_t>;
        using const_iterator = objects_vec_t::const_iterator;
    public:

        static object_layer parse(const xml_node& elt, const group* parent = nullptr);
        /**
         * @brief ObjectLayer constructor.
         */
        object_layer(std::string name, double opacity, bool visible, colori color, draw_order_t order,
                     int offsetx, int offsety, colori tint)
                : layer(std::move(name), opacity, visible),
                m_offsetx(offsetx),
                m_offsety(offsety),
                m_tint(tint),
                m_color(color),
                m_order(order)
        {
        }

        object_layer(object_layer&&) = default;

        /**
         * @brief Get the color used to display the objects.
         *
         * @return the color to display the objects
         */
        [[nodiscard]] const colori & color() const noexcept {
            return m_color;
        }

        [[nodiscard]] draw_order_t draw_order() const noexcept {
            return m_order;
        }
        /**
         * @brief Add an object.
         *
         * @param obj the object
         */
        void add(object_t obj) {
            m_objects.emplace_back(std::move(obj));
        }

        [[nodiscard]] const objects_vec_t& objects() const noexcept {
            return m_objects;
        }


    private:
        int m_offsetx;
        int m_offsety;
        colori m_tint;
        colori m_color;
        draw_order_t m_order;
        objects_vec_t m_objects;
    };
}

#endif
