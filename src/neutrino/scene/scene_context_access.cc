//
// Created by igor on 23/11/2026.
//
#include "scene_context_access.hh"

namespace neutrino::detail {
    layer_stack* scene_context_access::layer_stack_of(base_scene* scene) noexcept {
        return scene ? scene->m_layer_stack.get() : nullptr;
    }

    canvas* scene_context_access::canvas_of(base_scene* scene) noexcept {
        return scene ? scene->m_canvas.get() : nullptr;
    }

    const compositor* scene_context_access::compositor_of(base_scene* scene) noexcept {
        return scene ? &scene->m_compositor : nullptr;
    }

    painter_state_stack* scene_context_access::painter_state_stack_of(base_scene* scene) noexcept {
        return scene ? scene->m_painter_state_stack.get() : nullptr;
    }
}
