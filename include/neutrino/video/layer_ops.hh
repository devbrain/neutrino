//
// Created by igor on 20/11/2026.
//

/// @file layer_ops.hh
/// @brief The layers of the scene being rendered: pushing, popping, clearing (painter.md §2.5).

#pragma once

#include <neutrino/neutrino_export.h>
#include <neutrino/video/layer_id.hh>

namespace neutrino {
    /**
     * @brief Makes @p lid the current layer of the scene being rendered: what is drawn next
     *        goes onto it.
     *
     * Works only during a scene's render(), and reaches that scene's layers, so a helper can
     * push a layer without holding the scene. The first push of a layer in a frame clears it
     * unless it is redrawn on demand; pushing it again continues drawing on it. Pushes nest,
     * and every push is ended by a pop_layer() before render() returns. The painter state is
     * not touched (R39): a layer is where draws go, the state how.
     *
     * ENFORCE: a scene is being rendered; it declared layers (base_scene::declare_layer); and
     * @p lid is one of them — a layer of another scene, or layer_id{}, is refused.
     */
    NEUTRINO_EXPORT void push_layer(layer_id lid);

    /**
     * @brief Ends the most recent push_layer(): the layer below it is current again, or the
     *        scene's base layer. The painter state is not touched (R39).
     *
     * ENFORCE: a scene with layers is being rendered; something was pushed — the frame's own
     * push of the base layer does not count; and the bound target is still the layer's, so a
     * texture::bind() scope that outlived the layer fails here.
     */
    NEUTRINO_EXPORT void pop_layer();

    class layer_scope;

    /// @brief push_layer(@p lid) now; pop_layer() when the returned scope ends, also on an
    ///        exception. ENFORCE: as push_layer().
    NEUTRINO_EXPORT [[nodiscard]] layer_scope scoped_layer(layer_id lid);

    /**
     * @brief Ends a scoped_layer() push when it is destroyed.
     *
     * Movable, not assignable: scopes end in the reverse order of their creation, as pushes
     * and pops do, and assigning a newer scope over an older one would pop the newer push.
     * Ending one never throws: a failure is logged.
     */
    class NEUTRINO_EXPORT layer_scope {
        friend layer_scope scoped_layer(layer_id lid);

        public:
            /// @brief Takes @p other's push; @p other ends nothing.
            layer_scope(layer_scope&& other) noexcept;
            layer_scope& operator=(layer_scope&&) = delete;
            layer_scope(const layer_scope&) = delete;
            layer_scope& operator=(const layer_scope&) = delete;
            /// @brief Pops the push this scope owns, if it still owns one.
            ~layer_scope();
        private:
            /// push_layer(@p lid), owned by the new scope.
            explicit layer_scope(layer_id lid);
            /// True while the scope owns a push it has not ended.
            bool m_valid{false};
    };

    /**
     * @brief Clears what the scene being rendered draws on now: its current layer (the newest
     *        push, or the base layer) to the layer's clear colour — transparent, or opaque
     *        white for a @c mod layer — or, in a scene without layers, the scene's picture.
     *
     * For an on-demand layer, the way to start again; an every-frame layer is cleared at its
     * first push anyway. ENFORCE: a scene is being rendered.
     */
    NEUTRINO_EXPORT void clear();

    /**
     * @brief For an on-demand layer of the scene being rendered: true when it must be drawn in
     *        full — never drawn yet, or its contents lost to a resize, a change of logical
     *        presentation or a device reset — until the end of a frame that pushes it.
     *
     * One answer for the whole frame. Always false for an every-frame layer, which keeps
     * nothing. ENFORCE: a scene with layers is being rendered, and @p lid is one of them.
     */
    NEUTRINO_EXPORT [[nodiscard]] bool needs_redraw(layer_id lid);
}
