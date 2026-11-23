//
// Created by igor on 18/11/2026.
//

#pragma once

#include <neutrino/video/painter_state.hh>

namespace neutrino::detail {
    // The painter's clip follows the painter (R39): SDL keeps a clip per target, so it is put
    // on each target bound. A clip set straight on the renderer — a UI backend's — is kept
    // when the painter has none: only a texture just bound loses the clip it held. The
    // painter's clip is the current scene's (R48); with no scene current there is none.

    /// True when the current scene's painter state has a clip; false with no scene current.
    [[nodiscard]] bool painter_has_clip() noexcept;

    /// The clip of a texture just bound (texture::bind(), so layer pushes too): the painter's
    /// if it has one; otherwise none, so a pooled texture does not keep its last owner's.
    /// Nothing without a renderer.
    void clip_bound_texture();

    /// The clip of the target bound again when a renderer_target_scope ends: the painter's if
    /// it has one; none if it had one when the scope began, since the target held that clip
    /// and it is gone; otherwise the target keeps its own. Nothing without a renderer. Never
    /// throws: a failure is counted.
    void clip_after_scope(bool painter_clip_at_start) noexcept;
}
