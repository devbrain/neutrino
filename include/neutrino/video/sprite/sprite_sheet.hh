//
// Created by igor on 05/07/2026.
//

#pragma once

#include <compare>
#include <cstddef>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/sprite/texture_atlas.hh>

namespace neutrino {
    class sprite_sheet;

    /**
     * @brief Opaque handle to a visual entry inside a @ref sprite_sheet.
     *
     * A visual is static sprite metadata over one texture-atlas frame. It is not a
     * live world object; live sprite instances should use their own handle type.
     */
    class NEUTRINO_EXPORT sprite_visual_id {
        friend class sprite_sheet;
        friend struct std::hash <sprite_visual_id>;

        public:
            /**
             * @brief Construct an invalid visual handle.
             */
            sprite_visual_id() = default;
            sprite_visual_id(const sprite_visual_id&) = default;
            sprite_visual_id& operator =(const sprite_visual_id&) = default;
            sprite_visual_id(sprite_visual_id&&) = default;
            sprite_visual_id& operator =(sprite_visual_id&&) = default;

            /**
             * @brief Is this not the null sentinel?
             */
            [[nodiscard]] bool valid() const noexcept;

            bool operator ==(const sprite_visual_id& other) const = default;

            bool operator !=(const sprite_visual_id& other) const {
                return !(*this == other);
            }

            std::strong_ordering operator <=>(const sprite_visual_id& other) const = default;

        private:
            explicit sprite_visual_id(std::size_t value)
                : m_value(value) {
            }

            static constexpr std::size_t invalid_value = std::numeric_limits <std::size_t>::max();

            std::size_t m_value{invalid_value};
    };

    /**
     * @brief Static metadata that turns one texture rectangle into a logical sprite visual.
     *
     * @ref texture_rect is copied from the CPU atlas frame so higher sprite layers do
     * not need to ask the texture registry for source rectangles. @ref origin is the
     * draw anchor in frame-local pixels. Collision and gameplay bounds belong to the
     * physics/gameplay systems, not to the sprite sheet.
     */
    struct sprite_visual {
        /**
         * @brief Source rectangle in the sheet texture atlas.
         */
        rect texture_rect{0, 0, 0, 0};

        /**
         * @brief Draw anchor in frame-local pixels.
         */
        point origin{0, 0};
    };

    /**
     * @brief Logical sprite sheet over one uploaded texture atlas.
     *
     * The sheet gives atlas frame rectangles sprite-level meaning: texture rect,
     * origin, and optional names for asset/tool lookup. Names are stored at the sheet
     * level so the runtime visual entries remain compact.
     */
    class NEUTRINO_EXPORT sprite_sheet {
        public:
            /**
             * @brief Construct an empty sheet over an uploaded texture atlas.
             */
            explicit sprite_sheet(gpu_texture_atlas_id atlas);

            /**
             * @brief Construct a sheet over an uploaded atlas and copy CPU frame rects.
             *
             * One visual is created for every CPU atlas frame. The visual texture rect
             * is copied from @c cpu_texture_atlas_frame::atlas_rect and the origin is
             * left at @c {0,0}.
             */
            sprite_sheet(gpu_texture_atlas_id atlas, const cpu_texture_atlas& source);

            /**
             * @brief Texture atlas backing every visual in this sheet.
             */
            [[nodiscard]] gpu_texture_atlas_id atlas() const noexcept;

            /**
             * @brief Number of visual definitions stored in the sheet.
             */
            [[nodiscard]] std::size_t visual_count() const noexcept;

            /**
             * @brief Does @p id address a visual currently stored in this sheet?
             */
            [[nodiscard]] bool contains(sprite_visual_id id) const noexcept;

            /**
             * @brief Return the visual handle at a zero-based sheet index.
             *
             * This is useful for sheets built directly from CPU atlas frames, where the
             * default visual order matches the CPU frame order.
             *
             * @pre @p index must be less than @ref visual_count.
             */
            [[nodiscard]] sprite_visual_id visual_id(std::size_t index) const;

            /**
             * @brief Add an unnamed visual definition.
             *
             * @return Stable handle to the inserted visual.
             */
            sprite_visual_id add_visual(sprite_visual visual);

            /**
             * @brief Add a visual definition and bind a lookup name to it.
             *
             * Names are a convenience for asset loaders, scripts, and tools. Rebinding
             * an existing name replaces the previous association.
             */
            sprite_visual_id add_visual(std::string name, sprite_visual visual);

            /**
             * @brief Get a visual definition by handle.
             *
             * @pre @p id must satisfy @ref contains.
             */
            [[nodiscard]] const sprite_visual& visual(sprite_visual_id id) const;

            /**
             * @brief Bind or rebind a sheet-local name to an existing visual.
             *
             * @pre @p id must satisfy @ref contains.
             */
            void bind(std::string name, sprite_visual_id id);

            /**
             * @brief Find a visual handle by sheet-local name.
             *
             * @return The matching handle, or std::nullopt if no such name is bound.
             */
            [[nodiscard]] std::optional <sprite_visual_id> find(std::string_view name) const;

        private:
            gpu_texture_atlas_id m_atlas;
            std::vector <sprite_visual> m_visuals;
            std::unordered_map <std::string, sprite_visual_id> m_names;
    };

    /**
     * @brief Upload a CPU atlas and build a default sprite sheet from its frame rects.
     *
     * This is the sprite-level convenience wrapper around @ref register_atlas. It
     * uploads the texture atlas, creates one unnamed visual per CPU frame, and copies
     * each frame's atlas rectangle into @ref sprite_visual::texture_rect.
     */
    NEUTRINO_EXPORT sprite_sheet register_sprite_sheet(
        const cpu_texture_atlas& atlas,
        atlas_texture_format format = atlas_texture_format::automatic);
}

/**
 * @brief @c std::hash specialization so @ref neutrino::sprite_visual_id keys
 *        @c unordered_map / @c unordered_set.
 */
template<>
struct std::hash <neutrino::sprite_visual_id> {
    [[nodiscard]] std::size_t operator()(const neutrino::sprite_visual_id& id) const noexcept {
        return std::hash <std::size_t>{}(id.m_value);
    }
};
