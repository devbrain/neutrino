//
// Created by igor on 9/1/24.
//

#ifndef NEUTRINO_MODULES_VIDEO_TILE_PROPERTIES_HH
#define NEUTRINO_MODULES_VIDEO_TILE_PROPERTIES_HH

#include <vector>
#include <bsw/s11n/s11n.hh>
#include <neutrino/modules/video/types.hh>
#include <neutrino/neutrino_export.hh>

namespace neutrino {

    struct NEUTRINO_EXPORT tile_property {
        tile_property(const tile_property& other)
            : texture(other.texture),
              tile(other.tile) {
        }

        tile_property(tile_property&& other) noexcept
            : texture(other.texture),
              tile(other.tile) {
        }

        tile_property& operator=(const tile_property& other) {
            if (this == &other)
                return *this;
            texture = other.texture;
            tile = other.tile;
            return *this;
        }

        tile_property& operator=(tile_property&& other) noexcept {
            if (this == &other)
                return *this;
            texture = other.texture;
            tile = other.tile;
            return *this;
        }

        tile_property(detail::underlying_texture_type texture, detail::underlying_tile_type tile)
            : texture(texture),
              tile(tile) {
        }

        tile_property() = default;

        friend bool operator==(const tile_property& lhs, const tile_property& rhs) {
            return lhs.texture == rhs.texture
                   && lhs.tile == rhs.tile;
        }

        friend bool operator!=(const tile_property& lhs, const tile_property& rhs) {
            return !(lhs == rhs);
        }

        detail::underlying_texture_type texture {INVALID_TEXTURE_VALUE.value_of()};
        detail::underlying_tile_type tile {EMPTY_TILE_VALUE.value_of()};

        SERIALIZATION_SCHEMA(texture, tile)
    };

    struct NEUTRINO_EXPORT tiles_sequence {
        tiles_sequence(const tiles_sequence& other)
            : sequence(other.sequence) {
        }

        tiles_sequence(tiles_sequence&& other) noexcept
            : sequence(std::move(other.sequence)) {
        }

        tiles_sequence& operator=(const tiles_sequence& other) {
            if (this == &other)
                return *this;
            sequence = other.sequence;
            return *this;
        }

        tiles_sequence& operator=(tiles_sequence&& other) noexcept {
            if (this == &other)
                return *this;
            sequence = std::move(other.sequence);
            return *this;
        }

        tiles_sequence() = default;

        tiles_sequence(const tile_property& tp) {
            sequence.emplace_back(tp);
        }

        std::vector<tile_property> sequence;

        SERIALIZATION_SCHEMA(sequence)
    };


}

#endif
