//
// Created by igor on 05/07/2026.
//

#pragma once

#include <cstdint>

#include <neutrino/video/sprite/sprite_sheet.hh>

namespace neutrino {
    /**
     * @brief Render-time flip flags for sprite drawing.
     *
     * The sprite position supplied to drawing is always the visual origin/anchor
     * after the requested flips are applied. @ref diagonal uses Tiled's
     * orthogonal/isometric diagonal flag semantics: it is applied before
     * horizontal/vertical flags and swaps the bottom-left and top-right corners,
     * effectively an x/y axis swap.
     */
    enum class sprite_flip : std::uint8_t {
        none = 0,
        horizontal = 1u << 0u,
        vertical = 1u << 1u,
        diagonal = 1u << 2u
    };

    [[nodiscard]] constexpr sprite_flip operator |(sprite_flip lhs, sprite_flip rhs) noexcept {
        return static_cast <sprite_flip>(
            static_cast <std::uint8_t>(lhs) | static_cast <std::uint8_t>(rhs));
    }

    [[nodiscard]] constexpr sprite_flip operator &(sprite_flip lhs, sprite_flip rhs) noexcept {
        return static_cast <sprite_flip>(
            static_cast <std::uint8_t>(lhs) & static_cast <std::uint8_t>(rhs));
    }

    constexpr sprite_flip& operator |=(sprite_flip& lhs, sprite_flip rhs) noexcept {
        lhs = lhs | rhs;
        return lhs;
    }

    constexpr sprite_flip& operator &=(sprite_flip& lhs, sprite_flip rhs) noexcept {
        lhs = lhs & rhs;
        return lhs;
    }

    /**
     * @brief Sprite render state without world position.
     *
     * This is the component-like value a world/entity layer can store to describe
     * how an object should look as a sprite. Placement stays outside the sprite
     * system and is supplied to drawing separately.
     */
    struct sprite_appearance {
        /**
         * @brief Registered visual to draw.
         */
        sprite_visual_ref visual;

        /**
         * @brief Sprite-local render transform.
         */
        sprite_flip flip{sprite_flip::none};

        /**
         * @brief False means the appearance is skipped by draw helpers.
         */
        bool visible{true};
    };
}
