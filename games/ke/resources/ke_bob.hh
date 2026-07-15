//
// BOB sprite decoder (flag 0x0305) -> onyx_image surface.
// See docs/bob.md for the format specification.
//

#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <onyx_image/surface.hpp>
#include <sdlpp/video/palette.hh>
#include <neutrino/video/geometry_types.hh>

namespace rs {
    // Per-frame metadata parsed from a BOB block header.
    struct bob_frame {
        int width = 0;
        int height = 0;
        int x_off_a = 0;
        int y_off_a = 0;
        int x_off_b = 0;
        int y_off_b = 0;
    };

    // A parsed BOB resource: a frame count followed by packed, RLE-compressed,
    // column-interleaved 256-colour sprite frames plus a transparency mask.
    class bob_image {
        public:
            // Parse a BOB resource (the whole blob: 2-byte block count + packed blocks).
            // Returns false and sets `error` on a fundamental problem. A malformed
            // trailing block stops iteration but already-parsed frames are retained.
            bool parse(std::span <const std::uint8_t> data, std::string& error);

            [[nodiscard]] std::size_t frame_count() const noexcept { return frames_.size(); }
            [[nodiscard]] const bob_frame& frame(std::size_t index) const { return frames_[index]; }

            // Decode frame `index` into `surf` as rgba8888, applying a 256-colour SDL
            // palette; the alpha channel comes
            // from the BOB transparency mask. Returns false and sets `error` on failure.
            bool decode(std::size_t index,
                        sdlpp::const_palette_ref palette,
                        onyx_image::surface& surf,
                        std::string& error) const;

            // Decode every frame into a single rgba8888 atlas, laid out as a shelf-packed
            // tile sheet. Each frame's location is recorded on the surface via
            // set_subrect (kind = frame, user_tag = frame index). `max_width` bounds the
            // sheet width; `padding` is the gap in pixels between tiles.
            bool decode_atlas(sdlpp::const_palette_ref palette,
                              onyx_image::surface& surf,
                              std::string& error,
                              std::vector<neutrino::rect>& rects,
                              int max_width = 1024,
                              int padding = 1) const;

        private:
            // Decode a single frame into an rgba8888 pixel buffer.
            bool decode_frame_rgba(std::size_t index,
                                   sdlpp::const_palette_ref palette,
                                   std::vector <std::uint8_t>& rgba,
                                   int& width, int& height,
                                   std::string& error) const;

            std::vector <std::uint8_t> data_; // owned copy of the resource
            std::vector <std::pair <std::size_t, std::size_t>> blocks_; // (offset, size) into data_
            std::vector <bob_frame> frames_;
    };

    // A 256-entry grayscale SDL palette, for when no real palette is
    // available (matches the BOB grayscale fallback).
    [[nodiscard]] sdlpp::palette bob_grayscale_palette();
} // namespace ke
