//
// Created by igor on 20/11/2026.
//

#pragma once

#include <neutrino/scene/base_scene.hh>

namespace neutrino::detail {
    /**
     * @brief The scene manager's way to a scene's context, its layers, its canvas, its
     *        compositor and its painter state, which base_scene keeps private.
     *
     * A friend of base_scene, so the public header needs only forward declarations of
     * detail::layer_stack, detail::canvas and detail::painter_state_stack.
     */
    struct scene_context_access {
        /// @brief The layers of @p scene: null if it declared none, or if @p scene is null.
        [[nodiscard]] static layer_stack* layer_stack_of(base_scene* scene) noexcept;

        /// @brief The canvas of @p scene: null if @p scene is null.
        [[nodiscard]] static canvas* canvas_of(base_scene* scene) noexcept;

        /// @brief The compositor of @p scene: null if @p scene is null.
        [[nodiscard]] static const compositor* compositor_of(base_scene* scene) noexcept;

        /// @brief The painter state and saves of @p scene (R48): null if @p scene is null.
        [[nodiscard]] static painter_state_stack* painter_state_stack_of(base_scene* scene) noexcept;
    };
}
