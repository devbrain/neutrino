//
// Created by igor on 05/07/2026.
//

#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/sprite/sprite_appearance.hh>

#include <sdlpp/video/color.hh>

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
     * @brief Image metadata referenced by a world resource.
     *
     * The world module records where image data comes from and its declared dimensions.
     * Loading the pixels into CPU or GPU resources is handled by the texture/sprite layer.
     */
    struct world_image {
        std::string format;
        std::filesystem::path source;
        std::optional <sdlpp::color> transparent_color;
        unsigned width{};
        unsigned height{};
        std::vector <std::uint8_t> data;
    };

    /**
     * @brief One tile cell in a tile layer.
     */
    struct world_tile_cell {
        world_tile_id gid{};
        sprite_flip flip{sprite_flip::none};
        bool rotated_hex_120{};

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
        double offset_x{};
        double offset_y{};
        float parallax_x{1.0f};
        float parallax_y{1.0f};
        std::optional <sdlpp::color> tint;
    };

    /**
     * @brief A tile grid layer. Finite maps use cells; infinite maps use chunks.
     */
    struct world_tile_layer : world_layer_header {
        unsigned width{};
        unsigned height{};
        std::vector <world_tile_cell> cells;
        std::vector <world_tile_chunk> chunks;

        [[nodiscard]] const world_tile_cell& at(unsigned x, unsigned y) const;
        [[nodiscard]] world_tile_cell& at(unsigned x, unsigned y);
    };

    /**
     * @brief An image layer.
     */
    struct world_image_layer : world_layer_header {
        std::optional <world_image> image;
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
        double rotation{};
        bool visible{true};
        world_tile_id gid{};
        sprite_flip flip{sprite_flip::none};
        bool rotated_hex_120{};
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

    /**
     * @brief Terrain definition inside a tileset.
     */
    struct world_terrain : world_component {
        std::string name;
        int tile{-1};
    };

    /**
     * @brief One frame of a tile animation.
     */
    struct world_tile_animation_frame {
        world_local_tile_id tile{};
        std::chrono::milliseconds duration{};
    };

    /**
     * @brief Per-tile metadata inside a tileset.
     */
    struct world_tile : world_component {
        static constexpr unsigned invalid_terrain = static_cast <unsigned>(-1);

        world_local_tile_id id{};
        std::array <unsigned, 4> terrain{
            invalid_terrain,
            invalid_terrain,
            invalid_terrain,
            invalid_terrain
        };
        double probability{1.0};
        std::optional <world_image> image;
        std::optional <world_object_layer> objects;
        std::vector <world_tile_animation_frame> animation;
    };

    /**
     * @brief Wang color entry inside a wang set.
     */
    struct world_wang_color : world_component {
        sdlpp::color color;
        std::string name;
        int tile{-1};
        double probability{};
    };

    /**
     * @brief Wang tile entry inside a wang set.
     */
    struct world_wang_tile {
        enum index : std::size_t {
            top = 0,
            top_right = 1,
            right = 2,
            bottom_right = 3,
            bottom = 4,
            bottom_left = 5,
            left = 6,
            top_left = 7
        };

        std::array <unsigned, 8> wang_id{};
        world_local_tile_id tile{};
        sprite_flip flip{sprite_flip::none};
    };

    /**
     * @brief Wang set definition inside a tileset.
     */
    struct world_wang_set : world_component {
        std::string name;
        int tile{-1};
        std::vector <world_wang_color> colors;
        std::vector <world_wang_tile> tiles;
    };

    /**
     * @brief Optional grid metadata for isometric tilesets.
     */
    struct world_tileset_grid {
        bool orthogonal{true};
        unsigned width{};
        unsigned height{};
    };

    /**
     * @brief Tileset metadata and tile rectangles.
     */
    struct world_tileset : world_component {
        unsigned first_gid{};
        std::string name;
        unsigned tile_width{};
        unsigned tile_height{};
        unsigned spacing{};
        unsigned margin{};
        unsigned tile_count{};
        unsigned columns{};
        int offset_x{};
        int offset_y{};
        std::optional <world_image> image;
        std::optional <world_tileset_grid> grid;
        std::vector <world_terrain> terrains;
        std::vector <world_tile> tiles;
        std::vector <world_wang_set> wang_sets;

        [[nodiscard]] const world_tile* tile(world_local_tile_id id) const noexcept;
        [[nodiscard]] rect tile_rect(world_local_tile_id id) const;

        /**
         * @brief Whether this tileset owns the given global tile id.
         *
         * The empty gid (0) belongs to no tileset. When tile_count is zero the
         * upper bound is unknown and any gid at or above first_gid matches.
         */
        [[nodiscard]] bool contains(world_tile_id gid) const noexcept;

        /**
         * @brief Convert a global tile id to a tileset-local id.
         *
         * @throws std::out_of_range when gid is below first_gid (including the
         *         empty gid 0); use world::tileset_for to pick the owning tileset.
         */
        [[nodiscard]] world_local_tile_id to_local(world_tile_id gid) const;
        [[nodiscard]] world_tile_id to_global(world_local_tile_id id) const noexcept;
    };

    /**
     * @brief CPU-side tile world model.
     *
     * This is independent from TMX and independent from GPU texture registration.
     * Loaders and direct builders should populate this value type.
     */
    class NEUTRINO_EXPORT world : public world_component {
        public:
            world() = default;

            [[nodiscard]] const std::string& version() const noexcept;
            void set_version(std::string version);

            [[nodiscard]] world_orientation orientation() const noexcept;
            void set_orientation(world_orientation orientation) noexcept;

            [[nodiscard]] world_render_order render_order() const noexcept;
            void set_render_order(world_render_order order) noexcept;

            [[nodiscard]] unsigned width() const noexcept;
            [[nodiscard]] unsigned height() const noexcept;
            void set_size(unsigned width, unsigned height) noexcept;

            [[nodiscard]] unsigned tile_width() const noexcept;
            [[nodiscard]] unsigned tile_height() const noexcept;
            void set_tile_size(unsigned width, unsigned height) noexcept;

            [[nodiscard]] sdlpp::color background_color() const noexcept;
            void set_background_color(sdlpp::color color) noexcept;

            [[nodiscard]] unsigned hex_side_length() const noexcept;
            void set_hex_side_length(unsigned length) noexcept;

            [[nodiscard]] world_stagger_axis stagger_axis() const noexcept;
            void set_stagger_axis(world_stagger_axis axis) noexcept;

            [[nodiscard]] world_stagger_index stagger_index() const noexcept;
            void set_stagger_index(world_stagger_index index) noexcept;

            [[nodiscard]] bool infinite() const noexcept;
            void set_infinite(bool infinite) noexcept;

            void add_tileset(world_tileset tileset);
            void add_layer(world_layer layer);

            [[nodiscard]] const std::vector <world_tileset>& tilesets() const noexcept;
            [[nodiscard]] std::vector <world_tileset>& tilesets() noexcept;
            [[nodiscard]] const std::vector <world_layer>& layers() const noexcept;
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
            std::string m_version{"1.0"};
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
            std::vector <world_tileset> m_tilesets;
            std::vector <world_layer> m_layers;
    };
}
