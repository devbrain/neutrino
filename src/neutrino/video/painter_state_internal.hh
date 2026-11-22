//
// Created by igor on 18/11/2026.
//

#pragma once

#include <cstddef>

#include <neutrino/video/painter_state.hh>

namespace neutrino::detail {
    // The painter's clip follows the painter (R39): SDL keeps a clip per target, so it is put
    // on each target bound. A clip set straight on the renderer — a UI backend's — is kept
    // when the painter has none: only a texture just bound loses the clip it held.

    /// True when the painter has a clip; false before the painter state exists.
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

    /// How many push_state() saves are held now; 0 before the painter state exists.
    [[nodiscard]] std::size_t painter_save_depth() noexcept;

    /// Pops saves, newest first, until @p depth are left, each restoring the state it saved:
    /// what a scene's frame does when it ends with pushes not popped. Never throws: a failure
    /// is logged.
    void pop_painter_saves_to(std::size_t depth) noexcept;
}
