//
// Created by igor on 05/07/2026.
//

#pragma once

#include <cstdint>

#include <neutrino/video/flip_mode.hh>
#include <neutrino/video/sprite/sprite_sheet.hh>

namespace neutrino {
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
        flip_mode flip{flip_mode::none};

        /**
         * @brief False means the appearance is skipped by draw helpers.
         */
        bool visible{true};
    };
}
