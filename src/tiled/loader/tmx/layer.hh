//
// Created by igor on 20/07/2021.
//

#ifndef NEUTRINO_LAYER_HH
#define NEUTRINO_LAYER_HH

#include "component.hh"

namespace neutrino::tiled::tmx {
    /**
   * @brief A layer is a layer in the whole map.
   *
   * There are three kinds of layers: image layers, tile layers and object layers.
   */
    class layer : public component {
    public:
        /**
         * @brief Layer constructor.
         */
        layer(const std::string& name, double opacity, bool visible)
                : m_name(name), m_opacity(opacity), m_visible(visible)
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

    private:
        const std::string m_name;
        double m_opacity;
        bool m_visible;
    };
}

#endif
