//
// Created by igor on 07/07/2026.
//

#pragma once

#include <span>
#include <vector>
#include <cstdint>
#include <optional>

#include <sdlpp/video/surface.hh>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/sprite/cpu_texture_atlas.hh>

namespace neutrino {
    /**
     * @brief Where a single input rectangle was placed inside the atlas.
     *
     * One @ref placement is emitted per successfully packed input. Placements are
     * not necessarily in input order — use @ref input_index to map back to the
     * original @c rects span. @ref bounds carries the final pixel rectangle
     * (position and the original, un-inflated tile size); the gutter belt around
     * it lives just outside @ref bounds and is reserved (not part of the size).
     */
    struct placement {
        unsigned page; ///< Index into pack_result::pages of the atlas page holding this tile.
        uint32_t input_index; ///< Index of the source rectangle in the input span.
        rect bounds; ///< Placed rectangle: {x, y} on the page, {w, h} the original tile size.
    };

    /**
     * @brief Result of packing a set of rectangles into one or more atlas pages.
     *
     * @ref placements is ordered by @c (page, input_index): all of page 0's tiles
     * first (ascending input index), then page 1's, and so on — so a renderer can
     * walk one page's tiles contiguously and bind its texture once. Every input
     * index in @c [0, rects.size()) appears exactly once. @ref pages gives the
     * trimmed pixel extent of each page (the bounding box of its tiles including
     * their gutter belts), not the max-page cap — allocate atlas surfaces at
     * these sizes. An empty input yields empty @ref placements and empty
     * @ref pages.
     */
    struct pack_result {
        std::vector <placement> placements; ///< One entry per input, ordered by (page, input_index).
        std::vector <dim> pages; ///< Trimmed {width, height} extent of each atlas page.
    };

    /**
     * @brief Pack rectangles into atlas pages, spilling to new pages as needed.
     *
     * Each input is inflated by @p margin on every side (a gutter belt that
     * prevents texture bleed under linear filtering / non-integer scale), packed
     * with stb_rect_pack, and reported back at its un-inflated size. Inputs that
     * do not fit the current page spill onto a new page; there is no limit on the
     * page count.
     *
     * @param rects      Sizes to pack; only each rect's {w, h} is read, {x, y} are ignored.
     * @param max_width  Per-page width cap in pixels (typically the renderer's max texture size).
     * @param max_height Per-page height cap in pixels.
     * @param margin     Gutter reserved on every side of each tile, in pixels (>= 0).
     * @return Per-input placements and per-page trimmed extents (see @ref pack_result).
     *
     * @throws if @p max_width or @p max_height is not positive, if @p margin is
     *         negative, or if any single inflated rect (@c w + 2*margin, @c h +
     *         2*margin) exceeds the page cap — the latter can never fit any page.
     */
    [[nodiscard]] NEUTRINO_EXPORT pack_result pack_atlas(std::span <const rect> rects,
                                                         int max_width, int max_height, int margin);

    /**
     * @brief Pack rectangles using the renderer's max texture size as the page cap.
     *
     * Convenience overload that resolves the per-page cap from the active
     * renderer (via @c service_locator), falling back to 2048x2048 when no
     * renderer is available. Because the cap depends on the runtime GPU, results
     * are environment-dependent; tests wanting determinism should call the
     * explicit-cap overload above.
     *
     * @param rects  Sizes to pack (see the explicit-cap overload).
     * @param margin Gutter reserved on every side of each tile, in pixels (>= 0).
     */
    [[nodiscard]] NEUTRINO_EXPORT pack_result pack_atlas(std::span <const rect> rects, int margin);

    /**
     * @brief Composited atlas pages plus the input -> page/rect mapping.
     *
     * @ref pages holds one @ref cpu_texture_atlas per atlas page, in page order.
     * @ref placements maps each input surface to where it landed — entry's
     * @c input_index names the source, @c page / @c bounds locate it — ordered by
     * @c (page, input_index), the same contract as @ref pack_result. Together they
     * let a caller resolve "source image k -> which page, which rectangle" without
     * re-deriving the packing.
     */
    struct surface_atlases {
        std::vector <cpu_texture_atlas> pages;      ///< One composited atlas per page.
        std::vector <placement>         placements; ///< input_index -> {page, bounds}.
    };

    /**
     * @brief One packable region: a sub-rectangle of a source surface, plus
     *        optional per-pixel mask metadata to carry onto the output frame.
     *
     * @ref src's {w,h} is the tile size; its {x,y} locates the tile within
     * @ref surface (use {0,0,w,h} for a whole image). This is the neutral input to
     * @ref pack_regions that both @ref pack_surfaces (whole surfaces) and
     * @ref pack_atlases (existing atlas frames) build.
     */
    struct pack_region {
        const sdlpp::surface* surface;        ///< Source pixels (not owned).
        rect                  src;            ///< Sub-rect to pack; its {w,h} is the tile size.
        const bitmask*        mask = nullptr; ///< Optional mask copied onto the output frame.
    };

    /**
     * @brief Composite arbitrary surface sub-rectangles into atlas pages.
     *
     * The general primitive behind @ref pack_surfaces and @ref pack_atlases: packs
     * each region's @c src size with @ref pack_atlas, blits the sub-rect into its
     * slot as a straight copy in @p format (extruding the gutter belt), and carries
     * each region's @ref pack_region::mask onto the output frame. Distinct source
     * surfaces are converted to @p format at most once, so packing many regions
     * that share a source (e.g. every frame of one sheet) is cheap.
     *
     * @param regions    Regions to pack; @c placements[i] maps back to @c regions[i].
     * @param format     Pixel format of the produced pages (see @ref pack_surfaces).
     * @param max_width  Per-page width cap.
     * @param max_height Per-page height cap.
     * @param margin     Gutter reserved on every side of each tile, in pixels (>= 0).
     * @param generate   Mask policy for regions with no @ref pack_region::mask: if
     *                   set, a bitmask is evaluated from the source pixels (against
     *                   the original, pre-conversion surface). Regions that already
     *                   carry a mask keep it. Default: no generation.
     */
    [[nodiscard]] NEUTRINO_EXPORT surface_atlases pack_regions(
        std::span <const pack_region> regions,
        sdlpp::pixel_format_enum format,
        int max_width, int max_height, int margin,
        std::optional <cpu_texture_atlas_mask_options> generate = std::nullopt);

    /**
     * @brief Convenience @ref pack_regions: format from the first region's surface,
     *        per-page cap from the renderer (default 2048). Empty input -> empty.
     */
    [[nodiscard]] NEUTRINO_EXPORT surface_atlases pack_regions(
        std::span <const pack_region> regions, int margin,
        std::optional <cpu_texture_atlas_mask_options> generate = std::nullopt);

    /**
     * @brief Convenience @ref pack_regions: explicit @p format, per-page cap from the
     *        renderer. Use when the pages must be a canonical format (e.g. RGBA) rather
     *        than whatever the first source happens to be — a palettized source would
     *        otherwise yield a palettized page the GPU sprite path cannot sample.
     */
    [[nodiscard]] NEUTRINO_EXPORT surface_atlases pack_regions(
        std::span <const pack_region> regions,
        sdlpp::pixel_format_enum format, int margin,
        std::optional <cpu_texture_atlas_mask_options> generate = std::nullopt);

    /**
     * @brief Composite surfaces into one or more atlas pages in a target format.
     *
     * Packs @p images with @ref pack_atlas, allocates a page surface per page in
     * @p format, and blits each source into its slot as a straight (un-blended)
     * copy, extruding each tile's border into its gutter belt to prevent bleed
     * under linear filtering. Sources are converted to @p format on the way in, so
     * they need not share a pixel format.
     *
     * @param images     Source surfaces to pack; read-only.
     * @param format     Pixel format of the produced atlas pages. Must be
     *                   alpha-capable if the sources carry transparency, otherwise
     *                   the conversion drops alpha and gaps/gutter become opaque.
     * @param max_width  Per-page width cap (see @ref pack_atlas).
     * @param max_height Per-page height cap.
     * @param margin     Gutter reserved on every side of each tile, in pixels (>= 0).
     * @param generate   Optional mask policy — see @ref pack_regions. Raw surfaces
     *                   carry no mask, so this is the only way to get one here.
     * @return Composited pages and the per-input placement mapping.
     *
     * @throws (via @ref pack_atlas) on invalid args or a surface larger than the
     *         page cap; also if a page allocation or source conversion fails.
     */
    [[nodiscard]] NEUTRINO_EXPORT surface_atlases pack_surfaces(
        std::span <const sdlpp::surface> images,
        sdlpp::pixel_format_enum format,
        int max_width, int max_height, int margin,
        std::optional <cpu_texture_atlas_mask_options> generate = std::nullopt);

    /**
     * @brief Convenience @ref pack_surfaces: format from the first image, per-page
     *        cap from the renderer (default 2048). Empty input -> empty.
     */
    [[nodiscard]] NEUTRINO_EXPORT surface_atlases pack_surfaces(
        std::span <const sdlpp::surface> images, int margin,
        std::optional <cpu_texture_atlas_mask_options> generate = std::nullopt);

    [[nodiscard]] NEUTRINO_EXPORT surface_atlases pack_surfaces(
        std::span <const sdlpp::surface> images,
        sdlpp::pixel_format_enum format,
        int margin,
        std::optional <cpu_texture_atlas_mask_options> generate = std::nullopt);

    /**
     * @brief Repack the frames of several atlases into fewer atlas pages.
     *
     * Merges tilesheets: every frame of every input atlas becomes a region (its
     * pixels and mask) and all are repacked together into @p format pages — useful
     * for collapsing many small sheets into one texture. Frames are enumerated
     * atlas-major, so @c placements[i].input_index counts frames in the order
     * (atlas 0's frames, then atlas 1's, ...); the caller maps that back to
     * (atlas, frame) via the per-atlas frame counts.
     *
     * @param atlases    Source atlases whose frames are repacked; read-only.
     * @param format     Pixel format of the produced pages.
     * @param max_width  Per-page width cap.
     * @param max_height Per-page height cap.
     * @param margin     Gutter reserved on every side of each tile, in pixels (>= 0).
     * @param generate   Optional mask policy — see @ref pack_regions. Applies only
     *                   to source frames that carry no mask; existing masks pass
     *                   through unchanged.
     */
    [[nodiscard]] NEUTRINO_EXPORT surface_atlases pack_atlases(
        std::span <const cpu_texture_atlas> atlases,
        sdlpp::pixel_format_enum format,
        int max_width, int max_height, int margin,
        std::optional <cpu_texture_atlas_mask_options> generate = std::nullopt);

    /**
     * @brief Convenience @ref pack_atlases: format from the first atlas's surface,
     *        per-page cap from the renderer (default 2048). Empty input -> empty.
     */
    [[nodiscard]] NEUTRINO_EXPORT surface_atlases pack_atlases(
        std::span <const cpu_texture_atlas> atlases, int margin,
        std::optional <cpu_texture_atlas_mask_options> generate = std::nullopt);

    [[nodiscard]] NEUTRINO_EXPORT surface_atlases pack_atlases(
        std::span <const cpu_texture_atlas> atlases,
        sdlpp::pixel_format_enum format,
        int margin,
        std::optional <cpu_texture_atlas_mask_options> generate = std::nullopt);

}
