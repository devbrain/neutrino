//
// Created by igor on 12/07/2026.
//

#pragma once

/**
 * @file sprite_set.hh
 * @brief Built render resources for one @ref sprite_def, addressed by name.
 *
 * A name facade over a @ref render_bundle (which owns the atlas, sheet, and clip
 * animations and tears them down in order). @ref build_sprite_set uploads the def's
 * image as one atlas, creates a sheet with each named visual (grid-expanded + explicit,
 * trim/origin baked), and registers one animation per clip. It bakes **no** shared states
 * -- sprites use per-instance playheads (a `sprite_state` an actor spawns from a clip), so
 * @ref render_bundle::states stays empty.
 */

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/sprite/render_bundle.hh>
#include <neutrino/video/sprite/sprite_animation.hh>
#include <neutrino/video/sprite/sprite_def.hh>
#include <neutrino/video/sprite/sprite_sheet.hh>

namespace neutrino {
    /**
     * @brief Built render resources for one @ref sprite_def, addressed by name.
     *
     * A name facade over the owning @ref render_bundle: it maps visual and clip names to
     * the registered @ref sprite_visual_ref / @ref sprite_animation_id ids that
     * @ref build_sprite_set populated. The bundle owns the atlas, sheet, and animations
     * and tears them down in order.
     */
    class NEUTRINO_EXPORT sprite_set : public render_bundle {
        public:
            /// @brief The registered visual bound to @p name, or nullopt.
            [[nodiscard]] std::optional <sprite_visual_ref> visual(std::string_view name) const;

            /// @brief The registered animation bound to clip @p name, or nullopt.
            [[nodiscard]] std::optional <sprite_animation_id> clip(std::string_view name) const;

            /// Name lookups. Populated by @ref build_sprite_set; the animations are also
            /// owned (for teardown) in @ref render_bundle::animations.
            std::unordered_map <std::string, sprite_visual_ref>   visuals_by_name;
            std::unordered_map <std::string, sprite_animation_id> clips_by_name;
    };

    /**
     * @brief Build and register the render resources for one @ref sprite_def.
     *
     * Decodes @ref sprite_def::image and uploads it as a single atlas (the sheet image is
     * already packed -- sub-rects are referenced in place, not repacked); creates a sheet
     * with each resolved named visual (grid frames first, then explicit visuals which may
     * override a grid name; duplicate explicit visual or clip names are errors); bakes the
     * trim/origin formula; and registers one looping-configurable animation per clip.
     *
     * All decoding happens before any registration; if a later registration throws, the
     * partially built @ref sprite_set tears down what it already owns, so a failed build
     * leaks nothing.
     *
     * @pre An application must be initialized (registration touches the sprite manager).
     * @throws (via image loading) when the image cannot be decoded; on a duplicate
     *         explicit visual/clip name; or when a clip frame references an unknown visual.
     */
    [[nodiscard]] NEUTRINO_EXPORT sprite_set build_sprite_set(const sprite_def& def);
}
