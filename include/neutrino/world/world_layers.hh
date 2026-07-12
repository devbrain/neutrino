//
// Created by igor on 05/07/2026.
//

#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/world/world_common.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/sprite/sprite_appearance.hh>

#include <sdlpp/video/color.hh>

namespace neutrino {
    /**
     * @brief One tile cell in a tile layer.
     */
    struct world_tile_cell {
        world_tile_id gid{};
        sprite_flip flip{sprite_flip::none};
        /// Draw-time rotation in degrees (clockwise), composed with @ref flip.
        /// A TMX hexagonal 120-degree cell decodes to 120; loaders without hex
        /// rotation leave it 0.
        float rotation_degrees{0.0f};

        [[nodiscard]] bool empty() const noexcept {
            return gid == 0;
        }

        [[nodiscard]] bool horizontally_flipped() const noexcept {
            return (flip & sprite_flip::horizontal) == sprite_flip::horizontal;
        }

        [[nodiscard]] bool vertically_flipped() const noexcept {
            return (flip & sprite_flip::vertical) == sprite_flip::vertical;
        }

        [[nodiscard]] bool diagonally_flipped() const noexcept {
            return (flip & sprite_flip::diagonal) == sprite_flip::diagonal;
        }
    };

    /**
     * @brief Infinite-map tile chunk.
     */
    struct world_tile_chunk {
        int x{};
        int y{};
        int width{};
        int height{};
        std::vector <world_tile_cell> cells;
    };

    /**
     * @brief Common presentation metadata for all world layers.
     */
    struct world_layer_header : world_component {
        world_layer_id id{};
        std::string name;
        float opacity{1.0f};
        bool visible{true};
        world_point offset{0.0f, 0.0f}; // layer draw offset in world pixels
        float parallax_x{1.0f};
        float parallax_y{1.0f};
        std::optional <sdlpp::color> tint;
    };

    /**
     * @brief A tile grid layer. Finite maps use cells; infinite maps use chunks.
     */
    struct NEUTRINO_EXPORT world_tile_layer : world_layer_header {
        unsigned width{};
        unsigned height{};
        std::vector <world_tile_cell> cells;
        std::vector <world_tile_chunk> chunks;

        [[nodiscard]] const world_tile_cell& at(unsigned x, unsigned y) const;
        [[nodiscard]] world_tile_cell& at(unsigned x, unsigned y);
    };

    /**
     * @brief One frame of an animated image layer.
     */
    struct world_image_frame {
        world_image image;
        std::chrono::milliseconds duration{};
    };

    /**
     * @brief An image layer.
     *
     * A single static picture (@ref image), or -- when @ref frames is non-empty -- an
     * animation that cycles those frames on the shared clock (@ref image is then unused).
     */
    struct world_image_layer : world_layer_header {
        std::optional <world_image> image;
        std::vector <world_image_frame> frames;
    };

    /**
     * @brief Shared geometry and presentation data for world objects.
     */
    struct world_object_base : world_component {
        world_object_id id{-1};
        std::string name;
        std::string type;
        world_point origin{0.0f, 0.0f};
        double width{};
        double height{};
        double rotation{}; ///< Clockwise degrees; a TMX hex-120 tile object folds +120 in here.
        bool visible{true};
        world_tile_id gid{};
        sprite_flip flip{sprite_flip::none};
    };

    struct world_rectangle_object : world_object_base {};
    struct world_ellipse_object : world_object_base {};
    struct world_point_object : world_object_base {};

    struct world_polyline_object : world_object_base {
        std::vector <world_point> points;
    };

    struct world_polygon_object : world_polyline_object {};

    struct world_text_object : world_object_base {
        std::string font_family{"sans-serif"};
        int pixel_size{16};
        bool wrap{};
        sdlpp::color color{0, 0, 0, 255};
        bool bold{};
        bool italic{};
        bool underline{};
        bool strike{};
        bool kerning{true};
        world_text_halign halign{world_text_halign::left};
        world_text_valign valign{world_text_valign::top};
        std::string text;
    };

    using world_object = std::variant <
        world_rectangle_object,
        world_ellipse_object,
        world_point_object,
        world_polyline_object,
        world_polygon_object,
        world_text_object
    >;

    /**
     * @brief An object layer.
     */
    struct world_object_layer : world_layer_header {
        sdlpp::color color{160, 160, 164, 255};
        world_object_draw_order draw_order{world_object_draw_order::top_down};
        std::vector <world_object> objects;
    };

    using world_layer = std::variant <world_tile_layer, world_image_layer, world_object_layer>;
}
