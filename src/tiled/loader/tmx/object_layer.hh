//
// Created by igor on 20/07/2021.
//

#ifndef NEUTRINO_OBJECT_LAYER_HH
#define NEUTRINO_OBJECT_LAYER_HH

#include "object.hh"
#include "layer.hh"
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

        static object_layer parse(const xml_node& elt);
        /**
         * @brief ObjectLayer constructor.
         */
        object_layer(std::string name, double opacity, bool visible, const std::string color, draw_order_t order)
                : layer(std::move(name), opacity, visible), m_color(color), m_order(order)
        {
        }

        object_layer(object_layer&&) = default;

        /**
         * @brief Get the color used to display the objects.
         *
         * @return the color to display the objects
         */
        const std::string& color() noexcept {
            return m_color;
        }

        /**
         * @brief Add an object.
         *
         * @param obj the object
         */
        void add(object_t obj) {
            m_objects.emplace_back(std::move(obj));
        }

        /**
         * @brief Get the begin iterator on the objects.
         *
         * @return the begin iterator
         */
        [[nodiscard]] const_iterator begin() const noexcept {
            return m_objects.begin();
        }

        /**
         * @brief Get the end iterator on the objects.
         *
         * @return the end iterator
         */
        [[nodiscard]] const_iterator end() const noexcept {
            return m_objects.end();
        }

    private:
        const std::string m_color;
        const draw_order_t m_order;
        objects_vec_t m_objects;
    };
}

#endif
