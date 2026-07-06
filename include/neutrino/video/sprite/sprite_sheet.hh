//
// Created by igor on 05/07/2026.
//

#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/detail/hash.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/sprite/texture_atlas.hh>
#include <neutrino/video/sprite/detail/id_strong_type.hh>

namespace neutrino {
    class sprite_sheet;

    namespace details {
        struct sprite_sheet_id_tag;
    }

    /**
     * @brief Opaque handle to a registered @ref sprite_sheet.
     *
     * The handle identifies sprite-sheet metadata stored by the internal sprite
     * resource manager. It does not identify a world object and carries no position.
     */
    class NEUTRINO_EXPORT sprite_sheet_id
        : public details::id_strong_type <details::sprite_sheet_id_tag> {
        friend struct details::id_access;

        public:
            /**
             * @brief Construct an invalid sheet handle.
             */
            sprite_sheet_id() = default;

        private:
            explicit sprite_sheet_id(std::uint32_t value)
                : id_strong_type(value) {
            }
    };

    namespace details {
        struct sprite_visual_id_tag;
    }

    /**
     * @brief Opaque handle to a visual entry inside a @ref sprite_sheet.
     *
     * A visual is static sprite metadata over one texture-atlas frame. It is not a
     * live world object; runtime appearance stores @ref sprite_visual_ref values.
     */
    class NEUTRINO_EXPORT sprite_visual_id
        : public details::id_strong_type <details::sprite_visual_id_tag> {
        friend class sprite_sheet;
        friend struct std::hash <sprite_visual_id>;

        public:
            /**
             * @brief Construct an invalid visual handle.
             */
            sprite_visual_id() = default;

            /**
             * @brief Is this a visual handle minted by a sprite sheet?
             */
            [[nodiscard]] bool valid() const noexcept {
                return id_strong_type::valid() && m_owner != invalid_owner;
            }

            bool operator ==(const sprite_visual_id& other) const = default;

            bool operator !=(const sprite_visual_id& other) const {
                return !(*this == other);
            }

            std::strong_ordering operator <=>(const sprite_visual_id& other) const = default;

        private:
            static constexpr auto invalid_owner = std::numeric_limits <std::uint64_t>::max();

            sprite_visual_id(std::uint32_t value, std::uint64_t owner)
                : id_strong_type(value),
                  m_owner(owner) {
            }

            std::uint64_t m_owner{invalid_owner};
    };

    /**
     * @brief Complete registered-resource reference to one sprite visual.
     *
     * @ref sheet identifies the registered sheet resource and @ref visual identifies
     * the static visual inside that sheet. This is the value a higher gameplay layer
     * can store as "what this object looks like" without storing position.
     */
    struct sprite_visual_ref {
        /**
         * @brief Registered sheet containing the visual.
         */
        sprite_sheet_id sheet;

        /**
         * @brief Visual entry inside @ref sheet.
         */
        sprite_visual_id visual;

        /**
         * @brief Are both handles non-null sentinels?
         */
        [[nodiscard]] bool valid() const noexcept {
            return sheet.valid() && visual.valid();
        }

        bool operator ==(const sprite_visual_ref& other) const = default;

        bool operator !=(const sprite_visual_ref& other) const {
            return !(*this == other);
        }
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
            static std::uint64_t next_visual_owner() noexcept;

            gpu_texture_atlas_id m_atlas;
            std::uint64_t m_visual_owner{next_visual_owner()};
            std::vector <sprite_visual> m_visuals;
            std::unordered_map <
                std::string,
                sprite_visual_id,
                details::transparent_string_hash,
                std::equal_to <>> m_names;
    };

    /**
     * @brief Register an already-built sprite sheet.
     *
     * The sheet is copied/moved into the internal sprite resource manager and can be
     * addressed later through the returned handle.
     */
    NEUTRINO_EXPORT sprite_sheet_id register_sprite_sheet(sprite_sheet sheet);

    /**
     * @brief Upload a CPU atlas and register a default sprite sheet from its frame rects.
     *
     * This is the sprite-level convenience wrapper around @ref register_atlas. It
     * uploads the texture atlas, creates one unnamed visual per CPU frame, and copies
     * each frame's atlas rectangle into @ref sprite_visual::texture_rect.
     */
    NEUTRINO_EXPORT sprite_sheet_id register_sprite_sheet(
        const cpu_texture_atlas& atlas,
        atlas_texture_format format = atlas_texture_format::automatic);

    /**
     * @brief Unregister a sprite sheet.
     *
     * Invalid or already-erased handles are ignored. A registered sheet cannot be
     * unregistered while any registered animation frame or fixed runtime state
     * still references one of its visuals.
     *
     * @pre An application must be initialized.
     */
    NEUTRINO_EXPORT void unregister_sprite_sheet(sprite_sheet_id sheet);

    /**
     * @brief Build a registered visual reference from a sheet handle and visual index.
     *
     * This is the usual lookup path for sheets created directly from CPU atlas
     * frames, where visual order matches frame order.
     *
     * @pre @p sheet must identify a registered sheet and @p index must be in range.
     */
    NEUTRINO_EXPORT sprite_visual_ref visual_ref(sprite_sheet_id sheet, std::size_t index);

    /**
     * @brief Find a registered visual reference by sheet-local name.
     *
     * @return The matching registered visual reference, or std::nullopt when no name
     *         is bound in that sheet.
     *
     * @pre @p sheet must identify a registered sheet.
     */
    NEUTRINO_EXPORT std::optional <sprite_visual_ref> find_visual_ref(
        sprite_sheet_id sheet,
        std::string_view name);
}

/**
 * @brief @c std::hash specialization so @ref neutrino::sprite_sheet_id keys
 *        @c unordered_map / @c unordered_set.
 */
template<>
struct std::hash <neutrino::sprite_sheet_id>
    : std::hash <neutrino::details::id_strong_type <neutrino::details::sprite_sheet_id_tag>> {
};

/**
 * @brief @c std::hash specialization so @ref neutrino::sprite_visual_id keys
 *        @c unordered_map / @c unordered_set.
 */
template<>
struct std::hash <neutrino::sprite_visual_id> {
    [[nodiscard]] std::size_t operator()(const neutrino::sprite_visual_id& id) const noexcept {
        using base_type = neutrino::details::id_strong_type <neutrino::details::sprite_visual_id_tag>;
        auto result = std::hash <base_type>{}(static_cast <const base_type&>(id));
        neutrino::details::hash_combine(result, neutrino::details::hash_value(id.m_owner));
        return result;
    }
};

/**
 * @brief @c std::hash specialization so @ref neutrino::sprite_visual_ref keys
 *        @c unordered_map / @c unordered_set.
 */
template<>
struct std::hash <neutrino::sprite_visual_ref> {
    [[nodiscard]] std::size_t operator()(const neutrino::sprite_visual_ref& ref) const noexcept {
        auto result = std::hash <neutrino::sprite_sheet_id>{}(ref.sheet);
        neutrino::details::hash_combine(result, std::hash <neutrino::sprite_visual_id>{}(ref.visual));
        return result;
    }
};
