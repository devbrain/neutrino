//
// Created by igor on 12/07/2026.
//

#pragma once

/**
 * @file render_bundle.hh
 * @brief RAII owner of one set of registered render resources.
 *
 * A @ref render_bundle owns the GPU/sprite resources produced by a build --
 * @ref atlases, @ref sheets, @ref animations, and (for shared-state content like
 * animated tiles) @ref states -- and unregisters them in dependency order on
 * destruction. It is the shared owning core beneath the facades that add a lookup
 * table over it (@ref tileset_bundle by local tile id; a sprite set by name).
 *
 * Move-only: a copy would alias the ids and double-unregister. Both move operations
 * transfer ownership by *swapping* into an (empty) target, so the moved-from bundle
 * is provably empty and its destructor no-ops; move-assignment releases the target's
 * own resources first, so overwriting a live bundle never leaks.
 */

#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/sprite/texture_atlas.hh>
#include <neutrino/video/sprite/sprite_sheet.hh>
#include <neutrino/video/sprite/sprite_animation.hh>
#include <neutrino/video/sprite/sprite_state.hh>

namespace neutrino {
    struct NEUTRINO_EXPORT render_bundle {
        std::vector <gpu_texture_atlas_id> atlases;    ///< One uploaded atlas per packed page.
        std::vector <sprite_sheet_id>      sheets;     ///< One sheet per page (a sheet is single-atlas).
        std::vector <sprite_animation_id>  animations; ///< Registered animations, for teardown.
        std::vector <sprite_state_id>      states;     ///< Shared states (empty when instances own their own).

        render_bundle()                                = default;
        ~render_bundle();
        render_bundle(const render_bundle&)            = delete;
        render_bundle& operator=(const render_bundle&) = delete;
        render_bundle(render_bundle&&) noexcept;
        render_bundle& operator=(render_bundle&&) noexcept;

        /**
         * @brief Unregister every owned resource in dependency order and clear.
         *
         * Order is states -> animations -> sheets -> atlases, the reverse of the
         * registration dependencies (a state references an animation; an animation and
         * a fixed state reference sheet visuals; a sheet uses an atlas). Invalid handles
         * are ignored, so a repeat call (and the destructor) is a harmless no-op.
         */
        void release() noexcept;
    };
}
