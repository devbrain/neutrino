//
// Created by igor on 20/11/2026.
//

/// @file layer_ops.hh
/// @brief Pushing and popping the layers of the scene being rendered (painter.md §2.5).

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
}
