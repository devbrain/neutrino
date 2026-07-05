//
// Created by igor on 05/07/2026.
//

#pragma once

#include <cstddef>
#include <initializer_list>
#include <string_view>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/sprite/sprite_animation.hh>
#include <neutrino/video/sprite/sprite_appearance.hh>
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
