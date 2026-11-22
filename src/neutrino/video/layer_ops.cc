//
// Created by igor on 20/11/2026.
//

#include <neutrino/video/layer_ops.hh>

#include <exception>
#include <utility>

#include <failsafe/enforce.hh>
#include <failsafe/logger.hh>

#include "scene/scene_frame.hh"
#include "video/layer_stack.hh"

namespace neutrino {
    namespace {
        // The layers of the scene being rendered. ENFORCE: there is one, and it has layers.
        [[nodiscard]] detail::layer_stack& current_layers(const char* what) {
            ENFORCE(detail::scene_frame::rendering(), what, "outside a scene's render()");
            auto* layers = detail::scene_frame::get_current_layers_stack();
            ENFORCE(layers != nullptr, what, "in a scene that declared no layers");
            return *layers;
        }
    }

    void push_layer(layer_id lid) {
        current_layers("push_layer").push(lid);
    }

    void pop_layer() {
        current_layers("pop_layer").pop();
    }

    layer_scope scoped_layer(layer_id lid) {
        return layer_scope{lid};
    }

    layer_scope::layer_scope(layer_id lid) {
        push_layer(lid);
        m_valid = true;                        // only once pushed: a failed push owns nothing
    }

    layer_scope::layer_scope(layer_scope&& other) noexcept
        : m_valid(std::exchange(other.m_valid, false)) {
    }

    layer_scope::~layer_scope() {
        if (!m_valid) {
            return;
        }
        m_valid = false;
        try {
            pop_layer();
        } catch (const std::exception& e) {
            LOG_ERROR("cannot end a scoped_layer push:", e.what());
        } catch (...) {
            LOG_ERROR("cannot end a scoped_layer push");
        }
    }

    void clear() {
        ENFORCE(detail::scene_frame::rendering(), "clear outside a scene's render()");
        detail::scene_frame::clear_current();
    }

    bool needs_redraw(layer_id lid) {
        return current_layers("needs_redraw").needs_redraw(lid);
    }
}
