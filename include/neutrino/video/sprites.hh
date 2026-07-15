//
// Created by igor on 05/07/2026.
//

#pragma once

/**
 * @file sprites.hh
 * @brief Public entry point for the sprite system: aggregates the game-facing sprite
 *        headers and declares the appearance/animation builder functions.
 *
 * # The sprite system
 *
 * Sprites are drawn from **texture atlases** (packed images) as **visuals** (a sub-rect
 * with a pivot), optionally sequenced into **animations** played back by **states**. The
 * system separates *pure data* you can build/serialize/hash from *registered GPU
 * resources* addressed by opaque ids, and offers two ways to drive it: a low-level manual
 * path (register ids yourself) and a higher-level asset pipeline (a content-keyed cache of
 * built sprite sets with RAII handles). Drawing itself lives in
 * `<neutrino/video/draw.hh>` (@c draw_sprite) -- the sprite system stores no world
 * position; you supply it at draw time.
 *
 * # Vocabulary
 *
 * - **cpu_texture_atlas** -- a CPU surface plus frame rects (the asset side of an atlas).
 * - **gpu_texture_atlas_id** -- an uploaded texture (`register_atlas`).
 * - **sprite_visual** -- one static frame: a sub-rect of an atlas plus a pivot/origin.
 *   Addressed within a sheet by @ref sprite_visual_id, or globally by @ref sprite_visual_ref
 *   (sheet + visual).
 * - **sprite_sheet** (@ref sprite_sheet_id) -- named/indexed visuals over one atlas.
 * - **sprite_appearance** -- a visual + flip + visibility: *what* to draw, statically.
 * - **sprite_animation** (@ref sprite_animation_id) -- a sequence of timed appearances.
 * - **sprite_state** (@ref sprite_state_id) -- a runtime playhead: a fixed appearance, or a
 *   cursor advancing through an animation off the shared clock.
 * - **sprite_def** -- a pure, serializable asset description (image + grid/visuals + clips)
 *   with no GPU state; the authoring/loadable form.
 * - **sprite_set** -- the built resources for one @ref sprite_def (a @c render_bundle plus
 *   name->visual and name->clip lookups).
 * - **sprite_cache** / **sprite_set_handle** / **sprite_instance** -- a content-keyed,
 *   refcounted cache of sprite sets, an RAII lease that keeps one resident, and a
 *   per-entity playhead (a lease + its own state).
 *
 * # Theory of operation
 *
 * **Two representations.** Plain values (`cpu_texture_atlas`, `sprite_animation`,
 * `sprite_appearance`, `sprite_def`) are what you build, hash, and serialize. `register_*`
 * turns each into an opaque **id** owned by an internal, application-scoped *sprite
 * manager*; an id is invalid before the app is initialized and after you `unregister_*`
 * it. Ids never expose backend details -- treat them as value handles.
 *
 * **The animation clock is automatic.** The application update loop advances every
 * `sprite_state` each frame, so drawing a state resolves the current frame for you -- you
 * never tick states by hand. `switch_sprite_animation` changes clips only when different
 * (preserving elapsed time); `restart_sprite_animation` always restarts (for one-shots).
 *
 * **Ownership.** Manual ids are yours to `unregister_*`. A @c render_bundle owns a built
 * set of ids and releases them in dependency order (states -> animations -> sheets ->
 * atlases) on destruction; @ref sprite_set is such a bundle with a name facade. The cache
 * hands you RAII handles instead: a @ref sprite_set_handle keeps its set resident while any
 * copy lives, and a @ref sprite_instance unregisters its state first, then drops its lease.
 *
 * # Two paths
 *
 * - **Manual** -- `register_atlas` / `register_sprite_sheet` / `register_sprite_animation` /
 *   `create_sprite_state`, drawing by id. Full control; you own teardown. Good for
 *   procedural or one-off sheets, and for tiles.
 * - **Asset pipeline** -- author or load a @ref sprite_def, `acquire` it from a
 *   @ref sprite_cache, and `spawn` per-entity instances. Content-keyed (identical defs share
 *   one GPU build), RAII lifetime, named clips. The recommended path for actors.
 *
 * # Recipes
 *
 * A static sprite:
 * @code{.cpp}
 * sprite_sheet_id sheet = register_sprite_sheet(cpu_atlas); // upload + one visual per frame
 * sprite_visual_ref hero = visual_ref(sheet, 0);
 * draw_sprite(pos, hero);                                   // <neutrino/video/draw.hh>
 * @endcode
 *
 * A looping animation with a runtime state:
 * @code{.cpp}
 * sprite_sheet_id sheet = register_sprite_sheet(cpu_atlas);
 * sprite_animation_id walk = register_sprite_animation(
 *     make_sprite_animation(sheet, {0, 1, 2, 3}, sprite_animation_duration{100.0f}));
 * sprite_state_id st = create_sprite_state(walk);
 * // per frame (the app update loop advances st):
 * draw_sprite(pos, st);
 * // on teardown:
 * unregister_sprite_state(st);
 * @endcode
 *
 * The asset pipeline (recommended for actors):
 * @code{.cpp}
 * sprite_def def = load_aseprite_atlas(json);   // or build a sprite_def by hand
 * sprite_cache cache;                           // one long-lived cache per game/world
 * sprite_set_handle hero = cache.acquire(def);  // builds on miss, shares on hit
 * sprite_instance inst = hero.spawn("idle");    // per entity; keeps the set resident
 * // per frame:
 * draw_sprite(pos, inst.state());
 * // on input:
 * inst.switch_to("walk");                       // no-op if already walking
 * inst.restart("attack");                       // always restarts
 * // inst and hero are RAII: dropping them releases the state, then the lease.
 * @endcode
 *
 * Actors in a tile world draw through a @c render_layer that fills a @c sprite_batch
 * (`<neutrino/video/world/sprite_batch.hh>`), slotted among the map layers by
 * @c world_compositor -- depth-sorted, above the tiles.
 *
 * # Authoring a sprite_def
 *
 * A @ref sprite_def names an atlas image and how to frame and animate it, with no GPU
 * state -- so it can be written in code, serialized, or produced by @ref load_aseprite_atlas,
 * then content-hashed (@ref key_for) so identical defs share one cache build.
 *
 * **Image.** @ref sprite_def::image is a @ref world_image: pixels from a file
 * (`image_from_disk`), embedded bytes (`image_from_memory`), or an already-decoded surface
 * (`image_from_surface`, e.g. procedural art). @ref world_image::width / height are the
 * declared image size, from which a grid derives its frame count.
 *
 * **Frames come two ways, and may be combined:**
 * - A uniform @ref sprite_grid auto-slices the image into frames named `"0".."N-1"` (row
 *   major). `columns`/`count` of 0 derive from the image width; `margin`/`spacing` skip
 *   borders/gaps; the grid `origin` rule (@ref sprite_origin_rule, e.g. `bottom_center` for
 *   a feet pivot) picks each frame's pivot. Preview the expansion with @ref expand_grid.
 * - Explicit @ref sprite_visual_def entries name arbitrary sub-rects of a packed atlas, each
 *   with its own pivot and optional trim. An explicit visual may reuse a grid name to
 *   override that frame; duplicate explicit names are a build error.
 *
 * **Trim.** A packer often stores only a frame's non-empty rect. @ref sprite_visual_def::src
 * is that packed rect; @ref sprite_visual_def::origin is the pivot in the *untrimmed* frame;
 * @ref sprite_visual_def::source_size / @ref sprite_visual_def::trim_offset describe the
 * original frame. The build bakes the draw-space pivot as `origin - trim_offset` (see
 * @ref baked_visual_origin); untrimmed frames leave both unset.
 *
 * **Clips.** Each @ref sprite_clip_def is a named animation whose @ref sprite_frame_def steps
 * reference visuals *by name* (a grid `"3"` or an explicit `"idle"`), each with a duration
 * and optional flip; `loop` controls repeat. `spawn(clip)` / `switch_to` / `restart` play them.
 *
 * Authoring by hand (a grid sheet with one clip):
 * @code{.cpp}
 * sprite_def def;
 * def.image.source = image_from_disk{"hero.png"};
 * def.image.width  = 256;   // 8 columns of 32x32
 * def.image.height = 32;
 * def.grid = sprite_grid{.cell_w = 32, .cell_h = 32,
 *                        .origin = sprite_origin_rule::bottom_center};
 * def.clips.push_back(sprite_clip_def{
 *     .name = "walk",
 *     .frames = {{.visual = "0", .duration = sprite_animation_duration{100.0f}},
 *                {.visual = "1", .duration = sprite_animation_duration{100.0f}},
 *                {.visual = "2", .duration = sprite_animation_duration{100.0f}},
 *                {.visual = "3", .duration = sprite_animation_duration{100.0f}}},
 *     .loop = true});
 * // sprite_cache cache; auto h = cache.acquire(def); auto inst = h.spawn("walk");
 * @endcode
 *
 * # What this header pulls in
 *
 * The game-facing sprite headers (authoring, building/caching, sheets, runtime state,
 * atlas loading) plus the builder functions below. Low-level plumbing -- @c render_bundle,
 * @c atlas_packer, @c bitmask, @c image_identity -- is intentionally left out; engine
 * internals include those directly to keep incremental builds tight.
 */

#include <cstddef>
#include <initializer_list>
#include <string_view>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/sprite/atlas_loader.hh>
#include <neutrino/video/sprite/cpu_texture_atlas.hh>
#include <neutrino/video/sprite/sprite_animation.hh>
#include <neutrino/video/sprite/sprite_appearance.hh>
#include <neutrino/video/sprite/sprite_cache.hh>
#include <neutrino/video/sprite/sprite_def.hh>
#include <neutrino/video/sprite/sprite_set.hh>
#include <neutrino/video/sprite/sprite_sheet.hh>
#include <neutrino/video/sprite/sprite_state.hh>
#include <neutrino/video/sprite/texture_atlas.hh>

namespace neutrino {
    /**
     * @brief Required registered visual lookup by sheet-local name.
     *
     * This is the hard-fail counterpart to @ref find_visual_ref. Missing names are
     * treated as asset/configuration errors.
     *
     * @pre @p sheet must identify a registered sheet.
     * @pre @p name must be bound in that sheet.
     */
    NEUTRINO_EXPORT sprite_visual_ref visual_ref(sprite_sheet_id sheet, std::string_view name);

    /**
     * @brief Build a render appearance from a registered visual reference.
     */
    NEUTRINO_EXPORT sprite_appearance make_sprite_appearance(
        sprite_visual_ref visual,
        sprite_flip flip = sprite_flip::none,
        bool visible = true);

    /**
     * @brief Build a render appearance from a registered sheet visual index.
     *
     * @pre @p sheet must identify a registered sheet and @p index must be in range.
     */
    NEUTRINO_EXPORT sprite_appearance make_sprite_appearance(
        sprite_sheet_id sheet,
        std::size_t index,
        sprite_flip flip = sprite_flip::none,
        bool visible = true);

    /**
     * @brief Build a render appearance from a registered sheet visual name.
     *
     * @pre @p sheet must identify a registered sheet and @p name must be bound.
     */
    NEUTRINO_EXPORT sprite_appearance make_sprite_appearance(
        sprite_sheet_id sheet,
        std::string_view name,
        sprite_flip flip = sprite_flip::none,
        bool visible = true);

    /**
     * @brief Build one timed animation frame from an appearance.
     *
     * @pre @p duration must be positive.
     */
    NEUTRINO_EXPORT sprite_animation_frame make_sprite_animation_frame(
        sprite_appearance appearance,
        sprite_animation_duration duration);

    /**
     * @brief Build an animation from variable-duration frames.
     *
     * @pre @p frames must not be empty.
     * @pre Every frame duration must be positive.
     */
    NEUTRINO_EXPORT sprite_animation make_sprite_animation(
        std::initializer_list <sprite_animation_frame> frames,
        bool loop = true);

    /**
     * @brief Build an animation from appearances with one constant frame duration.
     *
     * @pre @p appearances must not be empty.
     * @pre @p frame_duration must be positive.
     */
    NEUTRINO_EXPORT sprite_animation make_sprite_animation(
        std::initializer_list <sprite_appearance> appearances,
        sprite_animation_duration frame_duration,
        bool loop = true);

    /**
     * @brief Build an animation from registered sheet visual indexes.
     *
     * Every selected visual uses the same duration and flip flags.
     *
     * @pre @p indexes must not be empty.
     * @pre Every index must be in range for @p sheet.
     * @pre @p frame_duration must be positive.
     */
    NEUTRINO_EXPORT sprite_animation make_sprite_animation(
        sprite_sheet_id sheet,
        std::initializer_list <std::size_t> indexes,
        sprite_animation_duration frame_duration,
        bool loop = true,
        sprite_flip flip = sprite_flip::none);

    /**
     * @brief Build an animation from registered sheet visual names.
     *
     * Every selected visual uses the same duration and flip flags.
     *
     * @pre @p names must not be empty.
     * @pre Every name must be bound in @p sheet.
     * @pre @p frame_duration must be positive.
     */
    NEUTRINO_EXPORT sprite_animation make_sprite_animation(
        sprite_sheet_id sheet,
        std::initializer_list <std::string_view> names,
        sprite_animation_duration frame_duration,
        bool loop = true,
        sprite_flip flip = sprite_flip::none);

    /**
     * @brief Build an animation from a contiguous range of sheet visual indexes.
     *
     * The selected indexes are [first_index, first_index + count).
     *
     * @pre @p count must be positive.
     * @pre Every selected index must be in range for @p sheet.
     * @pre @p frame_duration must be positive.
     */
    NEUTRINO_EXPORT sprite_animation make_sprite_animation_range(
        sprite_sheet_id sheet,
        std::size_t first_index,
        std::size_t count,
        sprite_animation_duration frame_duration,
        bool loop = true,
        sprite_flip flip = sprite_flip::none);
}
