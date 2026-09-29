//
// Created by igor on 29/09/2026.
//

#pragma once

/**
 * @file sprite_def_builder.hh
 * @brief Fluent builder for authoring and loading @ref sprite_def assets.
 *
 * Provides a chainable, ergonomic API for constructing sprite definitions from loose files,
 * in-memory buffers (e.g. extracted from resource archives or PAK files), streams, procedural
 * surfaces, or exported metadata documents (such as Aseprite JSON).
 */

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <istream>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/sprite/sprite_animation.hh>
#include <neutrino/video/sprite/sprite_appearance.hh>
#include <neutrino/video/sprite/sprite_def.hh>
#include <neutrino/world/world_common.hh>

namespace neutrino {
    class sprite_def_builder;

    /**
     * @brief Callback for resolving image bytes by filename or resource ID from a resource
     *        archive or virtual filesystem (e.g. PAK, ZIP, embedded bundle).
     */
    using sprite_resource_resolver = std::function <std::vector <std::uint8_t>(std::string_view name)>;

    /**
     * @brief Fluent sub-builder for constructing a single @ref sprite_clip_def.
     */
    class NEUTRINO_EXPORT sprite_clip_builder {
        public:
            sprite_clip_builder(sprite_def_builder* parent, std::string name, bool loop = true);

            /// @brief Append a frame by visual name.
            sprite_clip_builder& frame(
                std::string visual,
                sprite_animation_duration duration,
                sprite_flip flip = sprite_flip::none);

            /// @brief Append a frame by visual index (e.g. for grid frames "0", "1", ...).
            sprite_clip_builder& frame(
                std::size_t visual_index,
                sprite_animation_duration duration,
                sprite_flip flip = sprite_flip::none);

            /// @brief Set whether playback repeats after the last frame.
            sprite_clip_builder& loop(bool enable = true) noexcept;

            /// @brief Finish configuring this clip and return to the parent builder.
            sprite_def_builder& end_clip();

            /// @brief Synonym for @ref end_clip.
            sprite_def_builder& done() { return end_clip(); }

            /// @brief Commit the clip and build the resulting @ref sprite_def directly.
            [[nodiscard]] sprite_def build();

        private:
            sprite_def_builder* m_parent;
            sprite_clip_def     m_clip;
    };

    /**
     * @brief Fluent builder for constructing @ref sprite_def assets.
     *
     * Simplifies creating sprite definitions from loose files on disk, in-memory buffers
     * (e.g. extracted from resource archives or PAK files), streams, procedural surfaces,
     * or exported metadata documents (such as Aseprite JSON).
     */
    class NEUTRINO_EXPORT sprite_def_builder {
        public:
            sprite_def_builder() = default;

            // ------------------------------------------------------------------------
            // Image source (disk, memory/archive, stream, surface)
            // ------------------------------------------------------------------------

            /// @brief Set the image source to a file on disk.
            sprite_def_builder& from_file(std::filesystem::path path, unsigned width = 0, unsigned height = 0);
            sprite_def_builder& image_file(std::filesystem::path path, unsigned width = 0, unsigned height = 0) {
                return from_file(std::move(path), width, height);
            }

            /// @brief Set the image source from in-memory encoded bytes (PNG/BMP/etc.),
            /// e.g. read from a resource file or archive.
            sprite_def_builder& from_memory(std::vector <std::uint8_t> bytes, unsigned width = 0, unsigned height = 0);
            sprite_def_builder& from_memory(std::span <const std::uint8_t> bytes, unsigned width = 0, unsigned height = 0);
            sprite_def_builder& from_memory(std::string_view bytes, unsigned width = 0, unsigned height = 0);

            sprite_def_builder& image_bytes(std::vector <std::uint8_t> bytes, unsigned width = 0, unsigned height = 0) {
                return from_memory(std::move(bytes), width, height);
            }
            sprite_def_builder& image_bytes(std::span <const std::uint8_t> bytes, unsigned width = 0, unsigned height = 0) {
                return from_memory(bytes, width, height);
            }
            sprite_def_builder& image_bytes(std::string_view bytes, unsigned width = 0, unsigned height = 0) {
                return from_memory(bytes, width, height);
            }

            /// @brief Read encoded image bytes from an input stream until EOF.
            sprite_def_builder& from_stream(std::istream& is, unsigned width = 0, unsigned height = 0);
            sprite_def_builder& image_stream(std::istream& is, unsigned width = 0, unsigned height = 0) {
                return from_stream(is, width, height);
            }

            /// @brief Set the image from an already-decoded surface.
            sprite_def_builder& from_surface(std::shared_ptr <const sdlpp::surface> surface,
                                             std::optional <std::uint64_t> identity = std::nullopt);
            sprite_def_builder& from_surface(sdlpp::surface surface,
                                             std::optional <std::uint64_t> identity = std::nullopt);

            /// @brief Set raw @ref world_image_source directly.
            sprite_def_builder& image_source(world_image_source source, unsigned width = 0, unsigned height = 0);

            /// @brief Set the full @ref world_image struct directly.
            sprite_def_builder& image(world_image img);

            /// @brief Explicitly override declared atlas width and height.
            sprite_def_builder& image_size(unsigned width, unsigned height) noexcept;
            sprite_def_builder& image_size(dim size) noexcept;

            /// @brief Color keyed out as transparent, if any.
            sprite_def_builder& transparent_color(sdlpp::color color) noexcept;

            // ------------------------------------------------------------------------
            // Grid slicing
            // ------------------------------------------------------------------------

            /// @brief Set uniform grid slicing.
            sprite_def_builder& with_grid(sprite_grid grid);

            /// @brief Configure uniform grid slicing with individual parameters.
            sprite_def_builder& with_grid(
                unsigned cell_w,
                unsigned cell_h,
                sprite_origin_rule origin = sprite_origin_rule::top_left,
                unsigned margin = 0,
                unsigned spacing = 0,
                unsigned columns = 0,
                unsigned count = 0);

            /// @brief Remove grid slicing.
            sprite_def_builder& clear_grid() noexcept;

            // ------------------------------------------------------------------------
            // Visuals (frames)
            // ------------------------------------------------------------------------

            /// @brief Add a named visual frame with a pixel pivot.
            sprite_def_builder& add_visual(std::string name, rect src, point origin = point{0, 0});

            /// @brief Add a named visual frame with an origin rule (e.g. bottom_center, center).
            sprite_def_builder& add_visual(std::string name, rect src, sprite_origin_rule origin_rule);

            /// @brief Add a trimmed visual frame.
            sprite_def_builder& add_trimmed_visual(
                std::string name,
                rect packed_src,
                point pivot_in_untrimmed,
                dim source_size,
                point trim_offset);

            /// @brief Add a pre-constructed visual definition.
            sprite_def_builder& add_visual(sprite_visual_def visual);

            /// @brief Append multiple visual definitions.
            sprite_def_builder& add_visuals(std::vector <sprite_visual_def> visuals);

            // ------------------------------------------------------------------------
            // Clips (animations)
            // ------------------------------------------------------------------------

            /// @brief Start building a clip fluently using @ref sprite_clip_builder.
            [[nodiscard]] sprite_clip_builder clip(std::string name, bool loop = true);

            /// @brief Add a pre-constructed clip definition.
            sprite_def_builder& add_clip(sprite_clip_def clip);

            /// @brief Add a clip with uniform frame duration over a list of visual names.
            sprite_def_builder& add_clip(
                std::string name,
                std::initializer_list <std::string_view> visuals,
                sprite_animation_duration frame_duration,
                bool loop = true,
                sprite_flip flip = sprite_flip::none);

            /// @brief Add a clip with uniform frame duration over a list of grid visual indices.
            sprite_def_builder& add_clip(
                std::string name,
                std::initializer_list <std::size_t> visual_indices,
                sprite_animation_duration frame_duration,
                bool loop = true,
                sprite_flip flip = sprite_flip::none);

            /// @brief Add a clip over a contiguous range of grid indices [first_index, first_index + count).
            sprite_def_builder& add_clip_range(
                std::string name,
                std::size_t first_index,
                std::size_t count,
                sprite_animation_duration frame_duration,
                bool loop = true,
                sprite_flip flip = sprite_flip::none);

            // ------------------------------------------------------------------------
            // Metadata & Resource File integration
            // ------------------------------------------------------------------------

            /// @brief Parse Aseprite JSON metadata into this builder.
            /// Frames become explicit visuals, frameTags become clips, declared size is set.
            /// If no image source has been specified yet, it sets image_from_disk with the
            /// path from meta.image. If an image source was already specified (e.g. from_memory),
            /// that image source is preserved.
            sprite_def_builder& load_aseprite_metadata(std::string_view json_text);

            /// @brief Factory: create a builder initialized from Aseprite JSON text.
            [[nodiscard]] static sprite_def_builder from_aseprite_json(std::string_view json_text);

            /// @brief Factory: create a builder from Aseprite JSON text and in-memory image bytes
            /// (e.g. read from a resource file or archive).
            [[nodiscard]] static sprite_def_builder from_aseprite_json(
                std::string_view json_text,
                std::vector <std::uint8_t> image_bytes);

            [[nodiscard]] static sprite_def_builder from_aseprite_json(
                std::string_view json_text,
                std::span <const std::uint8_t> image_bytes);

            /// @brief Factory: create a builder from Aseprite JSON text using a resolver callback
            /// to load the image bytes from a resource file/archive.
            [[nodiscard]] static sprite_def_builder from_aseprite_json(
                std::string_view json_text,
                const sprite_resource_resolver& resolver);

            // ------------------------------------------------------------------------
            // Finalization & Reset
            // ------------------------------------------------------------------------

            /// @brief Reset builder to empty state.
            sprite_def_builder& reset() noexcept;

            /// @brief Finalize and return the built @ref sprite_def (copies state).
            [[nodiscard]] sprite_def build() const &;

            /// @brief Finalize and return the built @ref sprite_def (moves state).
            [[nodiscard]] sprite_def build() &&;

            /// @brief Implicit conversion to @ref sprite_def.
            operator sprite_def() const & { return build(); }
            operator sprite_def() && { return std::move(*this).build(); }

        private:
            friend class sprite_clip_builder;
            void commit_clip(sprite_clip_def clip);
            void validate() const;

            sprite_def m_def;
    };
}
