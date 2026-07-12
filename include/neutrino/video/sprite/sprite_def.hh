//
// Created by igor on 12/07/2026.
//

#pragma once

/**
 * @file sprite_def.hh
 * @brief Pure, loadable description of a sprite asset (§S3).
 *
 * A @ref sprite_def is to sprites what @ref world_tileset is to tiles: a value with no
 * GPU state that a builder/cache turns into a registered @ref sprite_set. It reuses the
 * @ref world_image source variant (disk/memory/surface) as its atlas source, so sprites
 * load from a file, embedded bytes, or a procedural surface.
 *
 * Frames come two ways, both carried as compact data: a uniform @ref sprite_grid
 * (auto-slices into frames named "0".."N-1"), and/or explicit named @ref sprite_visual_def
 * sub-rects (packed atlases, with per-visual origin and optional trim). Named @ref
 * sprite_clip_def animations reference visuals by name.
 */

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/sprite/image_identity.hh>
#include <neutrino/video/sprite/sprite_animation.hh>
#include <neutrino/video/sprite/sprite_appearance.hh>
#include <neutrino/world/content_key.hh>
#include <neutrino/world/world_common.hh>

namespace neutrino {
    /**
     * @brief Where a frame's pivot sits within its (untrimmed) cell. See @ref origin_for.
     */
    enum class sprite_origin_rule : std::uint8_t {
        top_left,     top_center,     top_right,
        center_left,  center,         center_right,
        bottom_left,  bottom_center,  bottom_right
    };

    /// @brief The pixel pivot a @ref sprite_origin_rule picks in a @p size cell.
    [[nodiscard]] NEUTRINO_EXPORT point origin_for(sprite_origin_rule rule, dim size) noexcept;

    /**
     * @brief Uniform grid slicing: auto-generates frames named "0".."count-1".
     *
     * @ref columns and @ref count default to derived-from-image (like a uniform tileset):
     * 0 columns => from the image width, 0 count => columns * rows.
     */
    struct sprite_grid {
        unsigned           cell_w{};
        unsigned           cell_h{};
        unsigned           columns{}; ///< 0 => derive from the image width.
        unsigned           count{};   ///< 0 => derive (columns * rows).
        unsigned           margin{};
        unsigned           spacing{};
        sprite_origin_rule origin{sprite_origin_rule::top_left};
    };

    /**
     * @brief A named frame: a sub-rect of the atlas image with a pivot and optional trim.
     *
     * @ref origin is the pivot in the *untrimmed* frame; @ref source_size / @ref trim_offset
     * describe how the packed @ref src was trimmed from that frame (both unset = untrimmed).
     * The build bakes `packed origin = origin - trim_offset` (§2 trim formula).
     */
    struct sprite_visual_def {
        std::string          name;
        rect                 src{};
        point                origin{0, 0};
        std::optional <dim>   source_size; ///< Untrimmed frame size (default = @ref src size).
        std::optional <point> trim_offset; ///< Trimmed rect's top-left in the untrimmed frame (default 0,0).
    };

    /// @brief One step of a clip: a visual (by name) shown for @ref duration, with @ref flip.
    struct sprite_frame_def {
        std::string               visual;
        sprite_animation_duration duration{0.0f};
        sprite_flip               flip{sprite_flip::none};
    };

    /// @brief A named animation over visuals.
    struct sprite_clip_def {
        std::string                    name;
        std::vector <sprite_frame_def> frames;
        bool                           loop{true};
    };

    /// @brief The pure sprite asset description. See @ref sprite_def.hh.
    struct sprite_def {
        world_image                     image;
        std::optional <sprite_grid>     grid;
        std::vector <sprite_visual_def> visuals;
        std::vector <sprite_clip_def>   clips;
    };

    /**
     * @brief Expand a grid over an image of @p image_size into indexed visual defs.
     *
     * Produces frames named "0".."count-1" in row-major order, deriving @ref
     * sprite_grid::columns / @ref sprite_grid::count from @p image_size when 0, each with
     * its @ref sprite_grid::origin rule applied over the cell size. Returns empty if the
     * cell size is 0 or no full cell fits.
     */
    [[nodiscard]] NEUTRINO_EXPORT std::vector <sprite_visual_def>
        expand_grid(const sprite_grid& grid, dim image_size);

    /**
     * @brief The origin baked into the packed visual: `logical_pivot − trim_offset`.
     *
     * @ref sprite_visual_def::origin is the pivot in the untrimmed frame; subtracting the
     * trim offset (default 0) puts it in the packed @ref sprite_visual_def::src local
     * space, where @c draw_sprite places it. Untrimmed frames reduce to `origin`.
     */
    [[nodiscard]] NEUTRINO_EXPORT point baked_visual_origin(const sprite_visual_def& v) noexcept;

    /**
     * @brief Content key of a @ref sprite_def, for the sprite cache.
     *
     * Folds the image identity (via @p ident), the grid params, and every visual and clip
     * in **declared order** (sequences, never map order), so two defs with the same content
     * share a bundle and any change yields a new key.
     */
    [[nodiscard]] NEUTRINO_EXPORT content_key key_for(const sprite_def& def, image_identifier& ident);
}
