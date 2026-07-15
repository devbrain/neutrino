//
// Created by igor on 05/07/2026.
//

#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/geometry_types.hh>

#include <sdlpp/video/color.hh>

namespace sdlpp {
    class surface;
}

namespace neutrino {
    /**
     * @brief Map orientation used by tile worlds.
     */
    enum class world_orientation {
        unknown,
        orthogonal,
        isometric,
        staggered,
        hexagonal
    };

    /**
     * @brief Tile render order for tile worlds.
     */
    enum class world_render_order {
        right_down,
        right_up,
        left_down,
        left_up
    };

    /**
     * @brief Staggered or hexagonal map stagger axis.
     */
    enum class world_stagger_axis {
        x,
        y
    };

    /**
     * @brief Staggered or hexagonal map stagger index.
     */
    enum class world_stagger_index {
        odd,
        even
    };

    /**
     * @brief Object-layer draw order.
     */
    enum class world_object_draw_order {
        top_down,
        index
    };

    /**
     * @brief Text horizontal alignment from Tiled-compatible object data.
     */
    enum class world_text_halign {
        left,
        center,
        right,
        justify
    };

    /**
     * @brief Text vertical alignment from Tiled-compatible object data.
     */
    enum class world_text_valign {
        top,
        center,
        bottom
    };

    using world_tile_id = std::uint32_t;
    using world_local_tile_id = std::uint32_t;
    using world_layer_id = std::uint32_t;
    using world_object_id = std::int64_t;
    using world_point = sdlpp::point <float>;
    using world_rect = sdlpp::rect <float>; ///< A rectangle in world pixels (float twin of @ref rect).

    /**
     * @brief Property value with an explicit type name for custom property kinds.
     */
    struct world_typed_string {
        std::string type;
        std::string value;

        [[nodiscard]] bool operator ==(const world_typed_string&) const = default;
    };

    /**
     * @brief Reference to another object stored as a property value.
     */
    struct world_object_reference {
        world_object_id id{};

        [[nodiscard]] bool operator ==(const world_object_reference&) const = default;
    };

    using world_property = std::variant <
        std::string,
        std::int64_t,
        bool,
        float,
        sdlpp::color,
        std::filesystem::path,
        world_object_reference,
        world_typed_string
    >;

    using world_property_map = std::map <std::string, world_property, std::less <>>;

    /**
     * @brief Named property bag shared by maps, layers, tilesets, tiles, and objects.
     */
    class NEUTRINO_EXPORT world_component {
        public:
            template <typename T>
            void set_property(std::string name, T&& value) {
                m_properties[std::move(name)] = world_property{std::forward <T>(value)};
            }

            [[nodiscard]] bool empty() const noexcept;
            [[nodiscard]] bool contains_property(std::string_view name) const noexcept;
            [[nodiscard]] const world_property* property(std::string_view name) const noexcept;
            [[nodiscard]] std::optional <world_property> get_property(std::string_view name) const;
            [[nodiscard]] const world_property_map& properties() const noexcept;

        private:
            world_property_map m_properties;
    };

    /**
     * @brief An image whose pixels come from a file on disk (decoded lazily at bind).
     */
    struct image_from_disk {
        std::filesystem::path source; ///< Path to the image file (decoded lazily at bind).
    };

    /**
     * @brief An image whose encoded (PNG/BMP/...) bytes live in memory (decoded lazily
     *        at bind). This is what an embedded TMX `<data>` image becomes.
     */
    struct image_from_memory {
        std::vector <std::uint8_t> bytes; ///< Encoded (PNG/BMP/...) image bytes, decoded lazily at bind.
    };

    /**
     * @brief An already-decoded image: a producer (procedural generation, an in-memory
     *        importer) hands over a surface.
     *
     * @ref identity is an optional stable content id the producer supplies so the
     * resource cache can dedup identical content; when absent the cache keys on the
     * surface instance, so identical-but-distinct surfaces do not dedup (still correct,
     * just not shared). Must reference *immutable* pixels — a surface mutated per frame
     * is keyed once and never re-uploaded; live content belongs to a render layer, not here.
     */
    struct image_from_surface {
        std::shared_ptr <const sdlpp::surface> pixels; ///< The already-decoded, immutable pixels.
        std::optional <std::uint64_t> identity;        ///< Stable content id for cache dedup; unset keys on the surface itself.
    };

    /// @brief Where an image's pixels come from. See @ref world_image.
    using world_image_source = std::variant <image_from_disk, image_from_memory, image_from_surface>;

    /**
     * @brief Image metadata referenced by a world resource.
     *
     * @ref source says where the pixels come from (disk / memory / a decoded surface);
     * @ref width / @ref height are the declared dimensions. Decoding into CPU or GPU
     * resources is handled by the texture/sprite layer, which visits @ref source.
     */
    struct world_image {
        world_image_source source{image_from_disk{}}; ///< Where the pixels come from (disk/memory/surface).
        std::optional <sdlpp::color> transparent_color; ///< Colour keyed out as transparent, if any.
        unsigned width{};  ///< Declared image width in pixels.
        unsigned height{}; ///< Declared image height in pixels.

        /// @brief True when no pixels are referenced (empty path / no bytes / null surface).
        [[nodiscard]] bool empty() const noexcept {
            if (const auto* d = std::get_if <image_from_disk>(&source)) {
                return d->source.empty();
            }
            if (const auto* m = std::get_if <image_from_memory>(&source)) {
                return m->bytes.empty();
            }
            if (const auto* s = std::get_if <image_from_surface>(&source)) {
                return s->pixels == nullptr;
            }
            return true;
        }
    };
}
