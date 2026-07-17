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
        world_tile_id gid{};                 ///< Global tile id drawn here; 0 is the empty cell (see @ref empty).
        sprite_flip flip{sprite_flip::none}; ///< Horizontal/vertical/diagonal flips applied to the tile graphic.
        /// Draw-time rotation in degrees (clockwise), composed with @ref flip.
        /// A TMX hexagonal 120-degree cell decodes to 120; loaders without hex
        /// rotation leave it 0.
        float rotation_degrees{0.0f};

        /// @brief True when the cell paints nothing (gid 0).
        [[nodiscard]] bool empty() const noexcept {
            return gid == 0;
        }

        /// @brief True if the tile graphic is mirrored horizontally.
        [[nodiscard]] bool horizontally_flipped() const noexcept {
            return (flip & sprite_flip::horizontal) == sprite_flip::horizontal;
        }

        /// @brief True if the tile graphic is mirrored vertically.
        [[nodiscard]] bool vertically_flipped() const noexcept {
            return (flip & sprite_flip::vertical) == sprite_flip::vertical;
        }

        /// @brief True if the tile graphic is flipped along its main diagonal (TMX anti-diagonal bit).
        [[nodiscard]] bool diagonally_flipped() const noexcept {
            return (flip & sprite_flip::diagonal) == sprite_flip::diagonal;
        }
    };

    /**
     * @brief Infinite-map tile chunk.
     */
    struct world_tile_chunk {
        int x{};      ///< Chunk origin x, in cells.
        int y{};      ///< Chunk origin y, in cells.
        int width{};  ///< Chunk width in cells.
        int height{}; ///< Chunk height in cells.
        std::vector <world_tile_cell> cells; ///< Row-major cells of the chunk.
    };

    /**
     * @brief Common presentation metadata for all world layers.
     */
    struct world_layer_header : world_component {
        world_layer_id id{};             ///< Unique layer id within the map.
        std::string name;                ///< Author-assigned layer name.
        float opacity{1.0f};             ///< Layer opacity in [0,1]; multiplied into drawn pixels.
        bool visible{true};              ///< Whether the layer is drawn.
        world_point offset{0.0f, 0.0f};  ///< Layer draw offset in world pixels, added to every element's position.
        float parallax_x{1.0f};          ///< Horizontal parallax factor relative to the camera (1 = moves with the map).
        float parallax_y{1.0f};          ///< Vertical parallax factor relative to the camera (1 = moves with the map).
        std::optional <sdlpp::color> tint; ///< Optional colour multiplied into the layer's pixels; unset leaves colour unchanged.
    };

    /**
     * @brief A tile grid layer. Finite maps use cells; infinite maps use chunks.
     */
    struct NEUTRINO_EXPORT world_tile_layer : world_layer_header {
        unsigned width{};  ///< Grid width in cells (finite maps); 0 for infinite maps.
        unsigned height{}; ///< Grid height in cells (finite maps); 0 for infinite maps.
        std::vector <world_tile_cell> cells;   ///< Row-major cells of a finite map (width x height); empty for infinite maps.
        std::vector <world_tile_chunk> chunks; ///< Sparse chunks of an infinite map; empty for finite maps.

        /**
         * @brief The cell at column @p x, row @p y of a finite layer's @ref cells grid.
         *
         * Indexes the row-major @ref cells buffer (size @ref width x @ref height); not
         * for infinite maps, which store their data in @ref chunks.
         *
         * @throws std::out_of_range when @p x >= @ref width or @p y >= @ref height.
         */
        [[nodiscard]] const world_tile_cell& at(unsigned x, unsigned y) const;

        /// @brief Mutable access to the cell at column @p x, row @p y.
        /// @throws std::out_of_range when @p x >= @ref width or @p y >= @ref height.
        [[nodiscard]] world_tile_cell& at(unsigned x, unsigned y);
    };

    /**
     * @brief One frame of an animated image layer.
     */
    struct world_image_frame {
        world_image image;                    ///< The frame's picture.
        std::chrono::milliseconds duration{}; ///< How long the frame is shown.
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
        world_object_id id{-1}; ///< Object id (-1 = unset).
        std::string name;       ///< Object name.
        std::string type;       ///< Author-assigned type/class string.
        world_point origin{0.0f, 0.0f}; ///< Position in world pixels (a tile object anchors bottom-left here).
        double width{};         ///< Object width in pixels.
        double height{};        ///< Object height in pixels.
        double rotation{}; ///< Clockwise degrees; a TMX hex-120 tile object folds +120 in here.
        bool visible{true};     ///< Whether the object is drawn.
        world_tile_id gid{};    ///< Tile gid for a tile object; 0 for a shape/text object.
        sprite_flip flip{sprite_flip::none}; ///< Flip applied to a tile object's graphic.
    };

    /// @brief A rectangular-region object; the rect is @ref world_object_base::origin
    ///        plus @ref world_object_base::width x @ref world_object_base::height.
    struct world_rectangle_object : world_object_base {};

    /// @brief An elliptical object inscribed in the object's bounding box.
    struct world_ellipse_object : world_object_base {};

    /// @brief A zero-size point object marking a single position (@ref world_object_base::origin).
    struct world_point_object : world_object_base {};

    /// @brief An open polyline; @ref points are vertices relative to @ref world_object_base::origin.
    struct world_polyline_object : world_object_base {
        std::vector <world_point> points; ///< Vertices, in order, relative to the object origin.
    };

    /// @brief A closed polygon; same vertex data as a polyline with an implicit closing edge.
    struct world_polygon_object : world_polyline_object {};

    /// @brief A text object: a string plus its font and layout attributes.
    struct world_text_object : world_object_base {
        std::string font_family{"sans-serif"};                  ///< Font family name.
        int pixel_size{16};                                     ///< Font size in pixels.
        bool wrap{};                                            ///< Whether text wraps within the object's width.
        sdlpp::color color{0, 0, 0, 255};                       ///< Text colour.
        bool bold{};                                            ///< Bold styling.
        bool italic{};                                          ///< Italic styling.
        bool underline{};                                       ///< Underline styling.
        bool strike{};                                          ///< Strikethrough styling.
        bool kerning{true};                                     ///< Whether kerning is applied.
        world_text_halign halign{world_text_halign::left};      ///< Horizontal alignment within the object box.
        world_text_valign valign{world_text_valign::top};       ///< Vertical alignment within the object box.
        std::string text;                                       ///< The displayed text.
    };

    /// @brief Any object stored in an object layer; the alternative selects the shape/kind.
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

    /// @brief Any layer in a world's layer list; the alternative selects the layer kind.
    using world_layer = std::variant <world_tile_layer, world_image_layer, world_object_layer>;
}
