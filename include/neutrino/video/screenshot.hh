//
// Created by igor on 22/11/2026.
//

/// @file screenshot.hh
/// @brief What the window shows, as an image (render_model.md R46).

#pragma once

#include <neutrino/neutrino_export.h>
#include <neutrino/video/image/image.hh>

namespace neutrino {
    /**
     * @brief What the window shows: every visible scene, the pinned one included, drawn
     *        again into a texture of the window's size and logical presentation, and read
     *        back opaque, as the screen shows it.
     *
     * Call it from anywhere a scene is not being drawn — fixed_update(), an event handler, a
     * test — with the window as the render target; `save_bmp(neutrino::screenshot(), path)`
     * writes it to disk. It costs one full extra draw of the scenes, only when called, and
     * relies on render() not advancing game state (it is not a clock). What a scene's
     * render() throws passes through.
     *
     * ENFORCE: a running application; not inside a scene's render() or compositor (frames
     * do not nest); no texture bound (compose(), bind()).
     */
    NEUTRINO_EXPORT [[nodiscard]] image screenshot();
}
