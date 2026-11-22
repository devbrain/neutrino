//
// Created by igor on 04/03/2026.
//

#include <neutrino/scene/base_scene.hh>

#include <utility>

#include <failsafe/enforce.hh>

#include "video/layer_stack.hh"
#include "video/canvas.hh"


namespace neutrino {
    base_scene::base_scene()
        : m_canvas(std::make_unique<detail::canvas>()) {
    }

    base_scene::~base_scene() = default;

    void base_scene::on_resize([[maybe_unused]] dim size) {
    }

    void base_scene::handle_action([[maybe_unused]] const sdlpp::event& ev) {
    }

    scene_flags base_scene::flags() const {
        return scene_flags::hides_below | scene_flags::pauses_below;
    }

    void base_scene::set_compositor(compositor c) {
        ENFORCE(static_cast <bool>(c), "set_compositor with an empty compositor; "
                                       "set_compositor(default_compositor) puts the default back");
        ENFORCE(!m_layer_stack || !m_layer_stack->combining(),
                "set_compositor inside the scene's own compositor");
        m_compositor = std::move(c);
    }

    const compositor& base_scene::get_compositor() const noexcept {
        return m_compositor;
    }

    layer_style& base_scene::style_of(layer_id lid) {
        ENFORCE(m_layer_stack != nullptr, "style_of in a scene that declared no layers");
        ENFORCE(lid != m_layer_stack->base_layer(),
                "the base layer is drawn as it is: declare a layer for drawing that must move or fade");
        return m_layer_stack->style(lid);
    }

    layer_id base_scene::declare_layer(const layer_settings& settings) {
        if (!m_layer_stack) {
            // No geometry yet: the first begin_frame() gives it the target's (R42).
            m_layer_stack = std::make_unique <detail::layer_stack>(detail::target_geometry{});
        }
        return m_layer_stack->declare(settings);
    }
}

