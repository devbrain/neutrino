//
// Created by igor on 20/07/2021.
//

#ifndef NEUTRINO_LAYER_HH
#define NEUTRINO_LAYER_HH

#include "component.hh"
#include "reader.hh"
#include <tuple>

namespace neutrino::tiled::tmx {
    /**
   * @brief A layer is a layer in the whole map.
   *
   * There are three kinds of layers: image layers, tile layers and object layers.
   */
    class layer : public component {
    public:
        // std::string name, double opacity, bool visible, int id
        static std::tuple<std::string, double, bool, int> parse(const reader& elt);
        /**
         * @brief Layer constructor.
         */
        layer(std::string name, double opacity, bool visible, int id)
                : m_name(std::move(name)),
                  m_opacity(opacity),
                  m_visible(visible),
                  m_id(id)
        {
        }

        [[nodiscard]] const std::string& name() const noexcept {
            return m_name;
        }


        [[nodiscard]] double opacity() const noexcept {
            return m_opacity;
        }


        [[nodiscard]] bool visible() const noexcept {
            return m_visible;
        }

        [[nodiscard]] int id() const noexcept {
            return m_id;
        }
    private:
        const std::string m_name;
        double m_opacity;
        bool m_visible;
        int m_id;
    };
}

#endif
