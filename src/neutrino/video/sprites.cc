//
// Created by igor on 05/07/2026.
//

#include <neutrino/video/sprites.hh>

#include <utility>
#include <vector>

#include <failsafe/enforce.hh>

namespace neutrino {
    namespace {
        void validate_animation_parts(std::size_t frame_count, sprite_animation_duration frame_duration) {
            ENFORCE(frame_count > 0);
            ENFORCE(frame_duration > sprite_animation_duration::zero());
        }
    }

    sprite_visual_ref visual_ref(sprite_sheet_id sheet, std::string_view name) {
        auto visual = find_visual_ref(sheet, name);
        ENFORCE(visual.has_value())("Sprite visual name is not bound");
        return *visual;
    }

    sprite_appearance make_sprite_appearance(
        sprite_visual_ref visual,
        sprite_flip flip,
        bool visible) {
        return sprite_appearance{
            .visual = visual,
            .flip = flip,
            .visible = visible
        };
    }

    sprite_appearance make_sprite_appearance(
        sprite_sheet_id sheet,
        std::size_t index,
        sprite_flip flip,
        bool visible) {
        return make_sprite_appearance(visual_ref(sheet, index), flip, visible);
    }

    sprite_appearance make_sprite_appearance(
        sprite_sheet_id sheet,
        std::string_view name,
        sprite_flip flip,
        bool visible) {
        return make_sprite_appearance(visual_ref(sheet, name), flip, visible);
    }

    sprite_animation_frame make_sprite_animation_frame(
        sprite_appearance appearance,
        sprite_animation_duration duration) {
        ENFORCE(duration > sprite_animation_duration::zero());
        return sprite_animation_frame{
            .appearance = appearance,
            .duration = duration
        };
    }

    sprite_animation make_sprite_animation(
        std::initializer_list <sprite_animation_frame> frames,
        bool loop) {
        ENFORCE(frames.size() > 0);
        return sprite_animation(std::vector <sprite_animation_frame>{frames.begin(), frames.end()}, loop);
    }

    sprite_animation make_sprite_animation(
        std::initializer_list <sprite_appearance> appearances,
        sprite_animation_duration frame_duration,
        bool loop) {
        validate_animation_parts(appearances.size(), frame_duration);

        std::vector <sprite_animation_frame> frames;
        frames.reserve(appearances.size());
        for (const auto& appearance : appearances) {
            frames.push_back(make_sprite_animation_frame(appearance, frame_duration));
        }
        return sprite_animation(std::move(frames), loop);
    }

    sprite_animation make_sprite_animation(
        sprite_sheet_id sheet,
        std::initializer_list <std::size_t> indexes,
        sprite_animation_duration frame_duration,
        bool loop,
        sprite_flip flip) {
        validate_animation_parts(indexes.size(), frame_duration);

        std::vector <sprite_animation_frame> frames;
        frames.reserve(indexes.size());
        for (auto index : indexes) {
            frames.push_back(make_sprite_animation_frame(
                make_sprite_appearance(sheet, index, flip),
                frame_duration));
        }
        return sprite_animation(std::move(frames), loop);
    }

    sprite_animation make_sprite_animation(
        sprite_sheet_id sheet,
        std::initializer_list <std::string_view> names,
        sprite_animation_duration frame_duration,
        bool loop,
        sprite_flip flip) {
        validate_animation_parts(names.size(), frame_duration);

        std::vector <sprite_animation_frame> frames;
        frames.reserve(names.size());
        for (auto name : names) {
            frames.push_back(make_sprite_animation_frame(
                make_sprite_appearance(sheet, name, flip),
                frame_duration));
        }
        return sprite_animation(std::move(frames), loop);
    }

    sprite_animation make_sprite_animation_range(
        sprite_sheet_id sheet,
        std::size_t first_index,
        std::size_t count,
        sprite_animation_duration frame_duration,
        bool loop,
        sprite_flip flip) {
        validate_animation_parts(count, frame_duration);

        std::vector <sprite_animation_frame> frames;
        frames.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            frames.push_back(make_sprite_animation_frame(
                make_sprite_appearance(sheet, first_index + i, flip),
                frame_duration));
        }
        return sprite_animation(std::move(frames), loop);
    }
}
