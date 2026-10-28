//
// Created by igor on 06/07/2026.
//

#pragma once

/**
 * @file image_loader.hh
 * @brief Image loading: built-in formats, custom decoders, and multi-image atlases.
 *
 * The rest of the engine loads images as a value, not an @c expected: every function
 * here turns a decode failure into a thrown exception, so callers get a ready
 * @c image (or @c image_atlas) or an error propagates.
 *
 * Three ways in, by what the file contains:
 *  - **One picture in a known format** (PNG, GIF, ...): @ref load_image. The format is
 *    sniffed by the registered decoders.
 *  - **One picture in a custom format**: implement an @ref image_decoder and either
 *    register it (@ref register_image_decoder, then @ref load_image finds it) or call it
 *    explicitly with @ref decode_image -- the way to go for headerless formats or ones that
 *    need context, such as an external palette.
 *  - **Many pictures in one file** (sprite sheets, animation frames, tile banks): write an
 *    @ref atlas_decoder and call @ref load_image_atlas. The decoder only describes the
 *    frames and writes their pixels; packing them into one sheet is the sink's job.
 */

#include <cstdint>
#include <utility>
#include <filesystem>
#include <functional>
#include <vector>
#include <memory>
#include <optional>
#include <span>

#include <neutrino/neutrino_export.h>
#include <sdlpp/image/image.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/palette.hh>

#include <failsafe/exception.hh>
#include <neutrino/video/image.hh>

namespace neutrino {
    /**
     * @brief Decode an image file into a surface.
     *
     * @param pth Filesystem path to the image (format inferred by sdlpp).
     * @return The decoded surface (ownership transferred to the caller).
     * @throws std::runtime_error carrying the decoder error if the file cannot
     *         be opened or decoded.
     */
    [[nodiscard]] inline image load_image(const std::filesystem::path& pth) {
        auto v = sdlpp::image::load(pth); // sdlpp-ok: implementation
        if (v) {
            return std::move(*v);
        }
        THROW_RUNTIME(v.error());
    }

    /**
     * @brief Decode an image from an in-memory encoded byte buffer into a surface.
     *
     * The counterpart to the path overload for images already resident in memory
     * (embedded assets, network/archive bytes); the format is sniffed from the
     * buffer contents.
     *
     * @param data Span over the encoded image bytes.
     * @return The decoded surface (ownership transferred to the caller).
     * @throws std::runtime_error carrying the decoder error if the bytes cannot
     *         be decoded.
     */
    [[nodiscard]] inline image load_image(std::span <const uint8_t> data) {
        auto v = sdlpp::image::load(data); // sdlpp-ok: implementation
        if (v) {
            return std::move(*v);
        }
        THROW_RUNTIME(v.error());
    }

    /**
     * @brief Write @p img to @p pth as a BMP file -- e.g. a capture_frame() screenshot.
     * @throws std::runtime_error carrying the error if the file cannot be written.
     */
    inline void save_bmp(const image& img, const std::filesystem::path& pth) {
        auto v = sdlpp::save_bmp(img, pth.string()); // sdlpp-ok: implementation
        if (!v) {
            THROW_RUNTIME(v.error());
        }
    }

    // ------------------------------------------------------------------------------------
    // Single images: custom decoders
    // ------------------------------------------------------------------------------------

    /**
     * @brief A single-image format plugin (onyx_image's decoder interface).
     *
     * Implement @c name(), @c extensions(), @c sniff() and @c decode(bytes, sink, opts).
     * A decoder may carry configuration in its members (an external palette, fixed
     * dimensions for a headerless format), which is why @ref decode_image takes an instance.
     */
    using image_decoder = onyx_image::decoder;

    /**
     * @brief What an @ref image_decoder writes into (onyx_image's surface interface).
     *
     * Note that @c image_sink::write_pixels takes @c x as a **byte offset** within the row,
     * not a pixel coordinate (unlike @ref atlas_sink::write_pixels).
     */
    using image_sink = onyx_image::surface;

    /// @brief Decode limits and requested colour output. Its packing fields are not used by
    ///        @ref load_image_atlas: frame placement is the @ref atlas_layout's job.
    using image_decode_options = onyx_image::decode_options;

    /// @brief Success or a decode error with a message; what decoders return.
    using image_decode_result = onyx_image::decode_result;

    /**
     * @brief Make @p dec available to @ref load_image (by content sniffing and by extension).
     *
     * Use it for self-describing formats, those @c sniff() can recognise. Registered decoders
     * live for the rest of the program. Formats that need context (a palette, dimensions)
     * cannot be sniffed: call them through @ref decode_image instead.
     *
     * @param dec The decoder; must not be null.
     */
    NEUTRINO_EXPORT void register_image_decoder(std::unique_ptr <image_decoder> dec);

    /**
     * @brief Decode @p bytes with the given decoder, bypassing the registry and sniffing.
     *
     * @param bytes The encoded image.
     * @param dec   The decoder to use, possibly configured (e.g. with an external palette).
     * @param opts  Size limits and colour output; defaults when omitted.
     * @return The decoded image.
     * @throws std::runtime_error carrying the decoder's message if decoding fails.
     */
    [[nodiscard]] NEUTRINO_EXPORT image decode_image(std::span <const std::uint8_t> bytes, const image_decoder& dec);
    /// @copydoc decode_image(std::span<const std::uint8_t>, const image_decoder&)
    [[nodiscard]] NEUTRINO_EXPORT image decode_image(std::span <const std::uint8_t> bytes, const image_decoder& dec,
                                                     const image_decode_options& opts);

    // ------------------------------------------------------------------------------------
    // Multi-image files: atlas decoders
    // ------------------------------------------------------------------------------------

    /**
     * @brief One frame of a multi-image file, as an @ref atlas_decoder declares it.
     */
    struct frame_info {
        dim size{0, 0};          ///< Frame size in pixels; both sides must be positive.
        point origin{0, 0};      ///< Pivot (hotspot) in frame-local pixels; may lie outside the frame.
        std::uint32_t index{0};  ///< The format's own id for the frame (block number, directory entry).
                                 ///< Carried unchanged into @ref image_region::index.
    };

    /**
     * @brief What the pixels of every frame are: their format, plus the palette data for
     *        indexed formats.
     *
     * Passed to @ref atlas_sink::set_frames. Requirements, checked by the engine's sinks:
     * @c colors is non-null exactly when @c pixels is an indexed format, and
     * @c transparent_index is set only for indexed formats.
     */
    struct frame_format {
        pixel_format pixels{pixel_format::RGBA8888}; ///< Format of the bytes given to @ref atlas_sink::write_pixels.

        /**
         * Required for indexed formats, null otherwise. Borrowed: valid only during
         * @ref atlas_sink::set_frames; a sink that needs the colours afterwards copies them
         * there. The decoder keeps ownership.
         */
        const palette* colors = nullptr;

        /**
         * The palette entry that marks transparent pixels, for indexed formats whose
         * transparency is a palette entry (a colour key such as brix's index 0, or GIF's
         * transparent index). Formats with real alpha write it in their pixels instead. An
         * index rather than a colour, so it is exact even when two entries hold the same
         * colour, and stays valid when the palette is swapped.
         */
        std::optional <std::uint8_t> transparent_index;
    };

    /**
     * @brief Destination of an @ref atlas_decoder: receives the frame list, then the pixels.
     *
     * The protocol has two phases, and a decoder must follow this order:
     *  1. @ref set_frames, exactly once, before anything else. It declares every frame and
     *     their shared @ref frame_format (palette included). The sink lays the frames out and
     *     allocates its storage here, so pixels can be written straight into place.
     *  2. @ref write_pixels, any number of times, for any frame, in any order.
     *
     * Frames are addressed by their position in the span given to @ref set_frames
     * (0 .. size-1), and pixels by frame-local coordinates: a decoder never learns where a
     * frame was placed.
     *
     * The engine ships two sinks that pack the frames onto one sheet: @ref rgba_atlas_sink
     * (what @ref load_image_atlas uses) and @ref indexed_atlas_sink. Implement this interface
     * only to send the frames somewhere else (tests, a contact sheet, texture pages), and drive
     * it with @ref decode_image_atlas.
     */
    class NEUTRINO_EXPORT atlas_sink {
        public:
            virtual ~atlas_sink();

            /**
             * @brief Declare every frame and the format they all share.
             *
             * @param frames All frames of the file, in the order the decoder will refer to them.
             *               Must not be empty. Borrowed for the duration of the call.
             * @param format The format of the bytes passed to @ref write_pixels, with the
             *               palette for indexed formats. One format for the whole file, since
             *               the frames end up on one sheet; a format with a palette per frame
             *               declares RGBA and expands the colours itself.
             * @return @c false if the sink cannot hold the frames (e.g. the layout exceeds the
             *         maximum sheet size, or the sink does not accept this format); the decoder
             *         must then stop and return an error.
             */
            virtual bool set_frames(std::span <const frame_info> frames, const frame_format& format) = 0;

            /**
             * @brief Write a horizontal run of pixels into one frame.
             *
             * @param frame Index into the span given to @ref set_frames.
             * @param x     First pixel of the run, in frame-local **pixels** (not bytes).
             * @param y     Row, frame-local.
             * @param bytes Pixel data in the format declared by @ref set_frames; its length
             *              fixes the run's width and must keep the run inside the frame.
             *
             * A frame's rows may be written in any order, whole or in parts. Pixels never
             * written keep the sink's initial value (transparent for the engine's sinks).
             */
            virtual void write_pixels(std::size_t frame, int x, int y, std::span <const std::uint8_t> bytes) = 0;
    };

    /**
     * @brief Where an @ref atlas_layout puts the frames.
     *
     * Requirements, checked by the engine's sinks: @c positions has exactly one entry per
     * frame, in frame order, and every frame (at its position, with its size) lies inside
     * @c sheet. Frames must not overlap.
     */
    struct atlas_layout_result {
        dim sheet;                     ///< Size of the sheet that holds all frames.
        std::vector <point> positions; ///< Top-left corner of each frame on the sheet.
    };

    /**
     * @brief A packing strategy: arranges frames on one sheet, given only their sizes.
     *
     * A pure function of its input. Write your own for special arrangements; the built-in
     * ones are @ref shelf_layout and @ref grid_layout.
     */
    using atlas_layout = std::function <atlas_layout_result (std::span <const frame_info>)>;

    /**
     * @brief Shelf packing: frames left to right in order, starting a new row when the next
     *        frame would pass @p max_width. The default layout.
     *
     * @param max_width Sheet width limit in pixels. A frame wider than this gets a row of its own.
     * @param padding   Empty pixels between neighbouring frames, to keep filtering from
     *                  bleeding one frame into the next.
     */
    NEUTRINO_EXPORT atlas_layout shelf_layout(int max_width = 2048, int padding = 1);

    /**
     * @brief Uniform grid: @p columns cells per row, every cell the size of the largest frame,
     *        each frame at its cell's top-left corner. For contact sheets and tilesets.
     *
     * @param columns Cells per row; must be positive.
     * @param padding Empty pixels between neighbouring cells.
     */
    NEUTRINO_EXPORT atlas_layout grid_layout(int columns, int padding = 0);

    /**
     * @brief A multi-image format: decodes @p bytes into @p sink following the
     *        @ref atlas_sink protocol.
     *
     * A plain function, so a decoder that needs context is a callable struct or a lambda
     * holding it (e.g. a decoder constructed with the game's palette). It returns an error
     * result rather than throwing; the loading functions turn that into an exception.
     */
    using atlas_decoder = std::function <image_decode_result(std::span <const std::uint8_t>,
                                                             atlas_sink& sink,
                                                             const image_decode_options& opts)>;

    /**
     * @brief One frame's place on an @ref image_atlas sheet.
     */
    struct image_region {
        rect bounds;             ///< The frame's pixels on the sheet.
        point origin;            ///< Pivot, frame-local (from @ref frame_info::origin).
        std::uint32_t index{};   ///< The format's own id (from @ref frame_info::index).
    };

    /**
     * @brief A decoded multi-image file: one sheet plus where each frame sits on it.
     *
     * @c regions is dense and in the order the decoder declared the frames, so
     * @c regions[i] is frame @c i. Look frames up by @ref image_region::index when game data
     * refers to them by the format's own numbering. The sheet is a CPU-side @ref image;
     * build sprites or a texture from it to draw.
     *
     * The sheet is RGBA8888 when built by @ref rgba_atlas_sink, or INDEX8 with the palette
     * and transparent index (as its colour key) attached when built by
     * @ref indexed_atlas_sink. Both kinds turn into a texture directly; an indexed sheet can
     * also be recoloured by replacing its palette first.
     */
    struct image_atlas {
        image pixels;                        ///< The sheet holding every frame.
        std::vector <image_region> regions;  ///< One entry per frame, in decoder order.
    };

    /**
     * @brief Packs the frames onto one RGBA8888 sheet: ready to draw.
     *
     * Accepts every input format:
     *  - indexed pixels are expanded through @ref frame_format::colors (copied into a lookup
     *    table in @ref set_frames), and @ref frame_format::transparent_index becomes alpha 0;
     *  - formats without alpha get alpha 255;
     *  - other formats are converted to RGBA8888.
     *
     * Gaps between frames and pixels never written are transparent (alpha 0).
     *
     * Single use: run one decode into it, then call @ref take.
     */
    class NEUTRINO_EXPORT rgba_atlas_sink final : public atlas_sink {
        public:
            /// @param layout How the frames are arranged on the sheet.
            explicit rgba_atlas_sink(atlas_layout layout = shelf_layout());
            ~rgba_atlas_sink() override;

            rgba_atlas_sink(const rgba_atlas_sink&) = delete;
            rgba_atlas_sink& operator=(const rgba_atlas_sink&) = delete;

            /**
             * @brief The finished atlas; leaves the sink empty.
             * @throws std::runtime_error if no frames were declared, or if called twice.
             */
            [[nodiscard]] image_atlas take();

            /// @throws std::runtime_error if called twice, if the layout's result violates
            ///         @ref atlas_layout_result, or if @p format violates @ref frame_format.
            ///         Returns @c false if the sheet cannot be allocated.
            bool set_frames(std::span <const frame_info> frames, const frame_format& format) override;
            /// @throws std::runtime_error if called before @ref set_frames, or if the run
            ///         falls outside the frame.
            void write_pixels(std::size_t frame, int x, int y, std::span <const std::uint8_t> bytes) override;

        private:
            struct impl;
            std::unique_ptr <impl> m_impl;
    };

    /**
     * @brief Packs the frames onto one INDEX8 sheet, keeping the palette indices.
     *
     * For when the colours are not final: palette effects, or the same sprites drawn with
     * different palettes. The resulting @ref image_atlas::pixels carries its own copy of
     * @ref frame_format::colors (made in @ref set_frames) and, if the format has one, the
     * transparent index as its colour key. The decoder's palette is never referenced after
     * @ref set_frames returns.
     *
     * Accepts indexed input only: @ref set_frames returns @c false for any other format.
     *
     * Gaps between frames and pixels never written hold the transparent index, or 0 if the
     * format has none.
     *
     * Single use: run one decode into it, then call @ref take.
     */
    class NEUTRINO_EXPORT indexed_atlas_sink final : public atlas_sink {
        public:
            /// @param layout How the frames are arranged on the sheet.
            explicit indexed_atlas_sink(atlas_layout layout = shelf_layout());
            ~indexed_atlas_sink() override;

            indexed_atlas_sink(const indexed_atlas_sink&) = delete;
            indexed_atlas_sink& operator=(const indexed_atlas_sink&) = delete;

            /**
             * @brief The finished atlas; leaves the sink empty.
             * @throws std::runtime_error if no frames were declared, or if called twice.
             */
            [[nodiscard]] image_atlas take();

            /// @throws std::runtime_error if called twice, if the layout's result violates
            ///         @ref atlas_layout_result, or if @p format violates @ref frame_format.
            ///         Returns @c false for a non-indexed format or if the sheet cannot be
            ///         allocated.
            bool set_frames(std::span <const frame_info> frames, const frame_format& format) override;
            /// @throws std::runtime_error if called before @ref set_frames, or if the run
            ///         falls outside the frame.
            void write_pixels(std::size_t frame, int x, int y, std::span <const std::uint8_t> bytes) override;

        private:
            struct impl;
            std::unique_ptr <impl> m_impl;
    };

    /**
     * @brief Decode a multi-image file and pack its frames onto one RGBA sheet.
     *
     * Shorthand for running @p dec into an @ref rgba_atlas_sink and taking the result. Use
     * @ref indexed_atlas_sink with @ref decode_image_atlas to keep palette indices instead.
     *
     * @param bytes  The encoded file.
     * @param dec    The format decoder.
     * @param layout How frames are arranged on the sheet; shelf packing by default.
     * @param opts   Size limits and colour output.
     * @return The sheet and one region per frame.
     * @throws std::runtime_error if the decoder fails or breaks the @ref atlas_sink protocol,
     *         or if @p layout returns a result that violates @ref atlas_layout_result.
     */
    [[nodiscard]] NEUTRINO_EXPORT image_atlas load_image_atlas(std::span <const std::uint8_t> bytes,
                                                               const atlas_decoder& dec,
                                                               atlas_layout layout = shelf_layout(),
                                                               const image_decode_options& opts = {});

    /**
     * @brief Run an atlas decoder into a sink of your own.
     *
     * The low-level path behind @ref load_image_atlas: no sheet is built, the frames go
     * wherever @p sink sends them.
     *
     * @param bytes The encoded file.
     * @param dec   The format decoder.
     * @param sink  Receives the frames and their pixels.
     * @param opts  Size limits and colour output.
     * @throws std::runtime_error carrying the decoder's message if decoding fails.
     */
    NEUTRINO_EXPORT void decode_image_atlas(std::span <const std::uint8_t> bytes,
                                            const atlas_decoder& dec,
                                            atlas_sink& sink,
                                            const image_decode_options& opts = {});
}
