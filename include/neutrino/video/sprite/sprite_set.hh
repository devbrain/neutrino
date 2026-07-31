//
// Created by igor on 12/07/2026.
//

#pragma once

/**
 * @file sprite_set.hh
 * @brief Built render resources for one @ref sprite_def, addressed by name or frame index.
 *
 * A facade over a @ref render_bundle (which owns the atlas, sheet, and clip animations and
 * tears them down in order). @ref build_sprite_set uploads the def's image as one atlas,
 * creates a sheet with each named visual (grid-expanded + explicit, trim/origin baked), and
 * registers one animation per clip. It bakes **no** shared states -- sprites use per-instance
 * playheads (a `sprite_state` an actor spawns from a clip), so @ref render_bundle::states
 * stays empty.
 *
 * The set is also **queryable**: besides resolving a visual by name or zero-based frame
 * index, it answers each frame's geometry (@ref sprite_set::frame_rect /
 * @ref sprite_set::origin) and the bounding size over a frame range
 * (@ref sprite_set::bounding_size). A consumer that needs a frame's pixel size (a collision
 * box, a layout extent) reads it from the built set and never has to keep the source
 * @ref sprite_def alive.
 */

#include <cstddef>
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
     * @brief Built render resources for one @ref sprite_def, addressed by name or frame index.
     *
     * A facade over the owning @ref render_bundle: it maps visual and clip names to the
     * registered @ref sprite_visual_ref / @ref sprite_animation_id ids that
     * @ref build_sprite_set populated, and exposes the sheet's frames by zero-based index.
     * Beyond resolving *what to draw*, it answers each frame's geometry (@ref frame_rect /
     * @ref origin) and the bounding size over a range (@ref bounding_size), so a caller
     * needing a frame's size does not retain the source def. The bundle owns the atlas,
     * sheet, and animations and tears them down in order.
     */
    class NEUTRINO_EXPORT sprite_set : public render_bundle {
        public:
            /// @brief The registered visual bound to @p name, or nullopt.
            [[nodiscard]] std::optional <sprite_visual_ref> visual(std::string_view name) const;

            /// @brief Number of visuals in the set (grid frames + explicit visuals).
            [[nodiscard]] std::size_t visual_count() const;

            /// @brief The visual at zero-based frame @p index, or nullopt if out of range.
            /// Grid frames come first in index order, so a sheet named "0".."N-1" maps
            /// index i to frame i -- no stringify-then-lookup.
            [[nodiscard]] std::optional <sprite_visual_ref> visual(std::size_t index) const;

            /// @brief Atlas rect (frame size + position) of the visual @p name, or nullopt.
            /// Lets a consumer read a frame's size without keeping the source def alive.
            [[nodiscard]] std::optional <rect> frame_rect(std::string_view name) const;

            /// @brief Atlas rect of the visual at frame @p index, or nullopt if out of range.
            [[nodiscard]] std::optional <rect> frame_rect(std::size_t index) const;

            /// @brief Pivot/origin of the visual @p name, or nullopt.
            [[nodiscard]] std::optional <point> origin(std::string_view name) const;

            /// @brief Pivot/origin of the visual at frame @p index, or nullopt if out of range.
            [[nodiscard]] std::optional <point> origin(std::size_t index) const;

            /// @brief Bounding size (max width x max height) over frames [@p first, end).
            /// A cell of this size fits any frame in range; @p first defaults to 0 (the
            /// whole set). Empty range yields {0, 0}.
            [[nodiscard]] dim bounding_size(std::size_t first = 0) const;

            /// @brief Bounding size over the frame range [@p first, @p first + @p count),
            /// clamped to the set.
            [[nodiscard]] dim bounding_size(std::size_t first, std::size_t count) const;

            /// @brief Resolved geometry of the visual @p name, or nullopt. Unlike @ref frame_rect
            /// (packed pixels only) this also answers the untrimmed authored frame and the pivot in
            /// both spaces, so a caller can align gameplay to art that the packer may re-trim.
            [[nodiscard]] std::optional <sprite_metrics> metrics(std::string_view name) const;

            /// @brief Resolved geometry of the visual at frame @p index, or nullopt if out of range.
            [[nodiscard]] std::optional <sprite_metrics> metrics(std::size_t index) const;

            /// @brief The registered animation bound to clip @p name, or nullopt.
            [[nodiscard]] std::optional <sprite_animation_id> clip(std::string_view name) const;

            // ---- required lookups -------------------------------------------------------
            // The nullopt-returning accessors above model a genuine question ("does this set
            // have such a frame?"). A CONFIGURATION INVARIANT -- "this sheet must contain the
            // frame this game is built around" -- is not a question, and answering it with
            // `.value_or(rect{})` yields zero geometry that propagates as a silently wrong
            // collision box or layout instead of a diagnosable failure. These abort at the
            // lookup, naming what was missing.

            /// @brief The registered visual bound to @p name. @throws if the set has no such visual.
            [[nodiscard]] sprite_visual_ref require_visual(std::string_view name) const;

            /// @brief The visual at zero-based frame @p index. @throws if out of range.
            [[nodiscard]] sprite_visual_ref require_visual(std::size_t index) const;

            /// @brief Atlas rect of the visual @p name. @throws if the set has no such visual.
            [[nodiscard]] rect require_frame_rect(std::string_view name) const;

            /// @brief Atlas rect of the visual at frame @p index. @throws if out of range.
            [[nodiscard]] rect require_frame_rect(std::size_t index) const;

            /// @brief Pivot/origin of the visual @p name. @throws if the set has no such visual.
            [[nodiscard]] point require_origin(std::string_view name) const;

            /// @brief Pivot/origin of the visual at frame @p index. @throws if out of range.
            [[nodiscard]] point require_origin(std::size_t index) const;

            /// @brief Resolved geometry of the visual @p name. @throws if the set has no such visual.
            [[nodiscard]] sprite_metrics require_metrics(std::string_view name) const;

            /// @brief Resolved geometry of the visual at frame @p index. @throws if out of range.
            [[nodiscard]] sprite_metrics require_metrics(std::size_t index) const;

            /// @brief The registered animation bound to clip @p name. @throws if there is no such clip.
            [[nodiscard]] sprite_animation_id require_clip(std::string_view name) const;

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
