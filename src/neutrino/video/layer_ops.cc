//
// Created by igor on 20/11/2026.
//

#include <neutrino/video/layer_ops.hh>

#include <failsafe/enforce.hh>

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
}
