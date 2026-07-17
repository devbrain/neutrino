//
// Created by igor on 05/07/2026.
//

#pragma once

#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/world/world_common.hh>
#include <neutrino/world/world_layers.hh>
#include <neutrino/world/world_tileset.hh>

#include <sdlpp/video/color.hh>

namespace neutrino {
    /**
     * @brief CPU-side tile world model.
     *
     * This is independent from TMX and independent from GPU texture registration.
     * Loaders and direct builders populate it through the setters below; renderers
     * read it back. Derives @ref world_component, so a map can carry custom properties.
     */
    class NEUTRINO_EXPORT world : public world_component {
        public:
            world() = default;

            /// @brief Map projection (orthogonal, isometric, staggered, hexagonal).
            [[nodiscard]] world_orientation orientation() const noexcept;
            void set_orientation(world_orientation orientation) noexcept;

            /// @brief Order in which tiles are laid out/drawn within a tile layer.
            [[nodiscard]] world_render_order render_order() const noexcept;
            void set_render_order(world_render_order order) noexcept;

            /// @brief Map width in tiles.
            [[nodiscard]] unsigned width() const noexcept;
            /// @brief Map height in tiles.
            [[nodiscard]] unsigned height() const noexcept;
            /// @brief Set the map size in tiles.
            void set_size(unsigned width, unsigned height) noexcept;

            /// @brief Width of one tile cell in pixels.
            [[nodiscard]] unsigned tile_width() const noexcept;
            /// @brief Height of one tile cell in pixels.
            [[nodiscard]] unsigned tile_height() const noexcept;
            /// @brief Set the tile-cell size in pixels.
            void set_tile_size(unsigned width, unsigned height) noexcept;

            /// @brief Map background colour (defaults to opaque black).
            [[nodiscard]] sdlpp::color background_color() const noexcept;
            void set_background_color(sdlpp::color color) noexcept;

            /// @brief Hex edge length in pixels; used only for hexagonal orientation.
            [[nodiscard]] unsigned hex_side_length() const noexcept;
            void set_hex_side_length(unsigned length) noexcept;

            /// @brief Axis the stagger offset is applied along (staggered/hex maps).
            [[nodiscard]] world_stagger_axis stagger_axis() const noexcept;
            void set_stagger_axis(world_stagger_axis axis) noexcept;

            /// @brief Which row/column indices are shifted (staggered/hex maps).
            [[nodiscard]] world_stagger_index stagger_index() const noexcept;
            void set_stagger_index(world_stagger_index index) noexcept;

            /// @brief True for an infinite map, whose tile layers store data in chunks
            ///        rather than a fixed @ref width x @ref height grid.
            [[nodiscard]] bool infinite() const noexcept;
            void set_infinite(bool infinite) noexcept;

            /// @brief Map-level parallax reference point in world pixels: the point that
            ///        stays fixed as parallax layers scroll. Default (0,0).
            [[nodiscard]] world_point parallax_origin() const noexcept;
            void set_parallax_origin(world_point origin) noexcept;

            /// @brief Append a tileset to the map (kept in the order added).
            void add_tileset(world_tileset tileset);
            /// @brief Append a layer; layers are stored back-to-front in draw order.
            void add_layer(world_layer layer);

            /// @brief All tilesets, in registration order.
            [[nodiscard]] const std::vector <world_tileset>& tilesets() const noexcept;
            /// @brief All tilesets, in registration order (mutable).
            [[nodiscard]] std::vector <world_tileset>& tilesets() noexcept;
            /// @brief All layers, back-to-front in draw order.
            [[nodiscard]] const std::vector <world_layer>& layers() const noexcept;
            /// @brief All layers, back-to-front in draw order (mutable).
            [[nodiscard]] std::vector <world_layer>& layers() noexcept;

            /**
             * @brief Find the tileset owning a global tile id.
             *
             * Follows the Tiled rule: the tileset with the highest first_gid
             * that is lower than or equal to gid. Returns nullptr for the empty
             * gid (0) or when no tileset qualifies.
             */
            [[nodiscard]] const world_tileset* tileset_for(world_tile_id gid) const noexcept;

            /**
             * @brief All layers of one concrete layer type, in map order.
             */
            template <typename T>
            [[nodiscard]] std::vector <const T*> layers_of() const {
                std::vector <const T*> result;
                for (const auto& layer : m_layers) {
                    if (const auto* typed = std::get_if <T>(&layer)) {
                        result.push_back(typed);
                    }
                }
                return result;
            }

            [[nodiscard]] std::vector <const world_object_layer*> object_layers() const;
            [[nodiscard]] std::vector <const world_tile_layer*> tile_layers() const;
            [[nodiscard]] std::vector <const world_image_layer*> image_layers() const;

        private:
            world_orientation m_orientation{world_orientation::unknown};
            world_render_order m_render_order{world_render_order::right_down};
            unsigned m_width{};
            unsigned m_height{};
            unsigned m_tile_width{};
            unsigned m_tile_height{};
            sdlpp::color m_background_color{0, 0, 0, 255};
            unsigned m_hex_side_length{};
            world_stagger_axis m_stagger_axis{world_stagger_axis::y};
            world_stagger_index m_stagger_index{world_stagger_index::odd};
            bool m_infinite{};
            world_point m_parallax_origin{0.0f, 0.0f};
            std::vector <world_tileset> m_tilesets;
            std::vector <world_layer> m_layers;
    };
}
