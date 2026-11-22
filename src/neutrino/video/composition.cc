//
// Created by igor on 22/11/2026.
//

#include <neutrino/video/composition.hh>

#include "video/layer_stack.hh"

namespace neutrino {
    composition::composition(detail::layer_stack& layers, std::span <const neutrino::texture* const> below) noexcept
        : m_layers(layers), m_below(below) {
    }

    std::span <const layer_id> composition::layers() const noexcept {
        return m_layers.layers();
    }

    bool composition::has_content(layer_id lid) const {
        return m_layers.has_content(lid);
    }

    void composition::draw_layer(layer_id lid) {
        m_layers.draw_layer(lid, m_layers.style(lid));
    }

    void composition::draw_layer(layer_id lid, const layer_style& settings) {
        m_layers.draw_layer(lid, settings);
    }

    void composition::draw_layers(layer_id first, layer_id last) {
        m_layers.draw_layers(first, last);
    }

    const neutrino::texture* composition::texture(layer_id lid) const {
        return m_layers.target(lid);
    }

    std::span <const neutrino::texture* const> composition::below() const noexcept {
        return m_below;
    }

    void default_compositor(composition& c) {
        for (const layer_id id : c.layers()) {    // declaration order
            if (c.has_content(id)) {              // pushed this frame, or on demand and not needing a redraw
                c.draw_layer(id);                 // onto the canvas, with its style
            }
        }
    }
}
