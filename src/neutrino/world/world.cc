//
// Created by igor on 05/07/2026.
//

#include <neutrino/world/world.hh>

#include <failsafe/exception.hh>

#include <limits>

namespace neutrino {
    bool world_component::empty() const noexcept {
        return m_properties.empty();
    }

    bool world_component::contains_property(std::string_view name) const noexcept {
        return m_properties.find(name) != m_properties.end();
    }

    const world_property* world_component::property(std::string_view name) const noexcept {
        const auto it = m_properties.find(name);
        if (it == m_properties.end()) {
            return nullptr;
        }
        return &it->second;
    }

    std::optional <world_property> world_component::get_property(std::string_view name) const {
        const auto* prop = property(name);
        if (prop == nullptr) {
            return std::nullopt;
        }
        return *prop;
    }

    const world_property_map& world_component::properties() const noexcept {
        return m_properties;
    }

    const world_tile_cell& world_tile_layer::at(unsigned x, unsigned y) const {
        if (x >= width || y >= height) {
            THROW_OUT_OF_RANGE("world_tile_layer coordinates (", x, ",", y,
                               ") are out of range for size", width, "x", height);
        }
        return cells.at(static_cast <std::size_t>(y) * width + x);
    }

    world_tile_cell& world_tile_layer::at(unsigned x, unsigned y) {
        return const_cast <world_tile_cell&>(
            static_cast <const world_tile_layer&>(*this).at(x, y));
    }

    const world_tile* world_tileset::tile(world_local_tile_id id) const noexcept {
        for (const auto& item : tiles) {
            if (item.id == id) {
                return &item;
            }
        }
        return nullptr;
    }

    rect world_tileset::tile_rect(world_local_tile_id id) const {
        if (!image) {
            THROW_LOGIC("tileset has no image");
        }
        if (tile_width == 0 || tile_height == 0) {
            THROW_LOGIC("tileset tile size is zero");
        }
        if (tile_count != 0 && id >= tile_count) {
            THROW_OUT_OF_RANGE("tileset tile id", id, "is out of range for tile count", tile_count);
        }
        const auto image_width = image->width;
        if (image_width < 2 * margin + tile_width) {
            THROW_LOGIC("tileset image is too small for the tile size and margin");
        }
        const auto image_height = image->height;
        if (image_height < 2 * margin + tile_height) {
            THROW_LOGIC("tileset image is too small for the tile size and margin");
        }
        const auto usable_width = image_width - 2 * margin + spacing;
        const auto stride_x = tile_width + spacing;
        const auto computed_columns = stride_x == 0 ? 0 : usable_width / stride_x;
        const auto actual_columns = columns == 0 ? computed_columns : columns;
        if (actual_columns == 0) {
            THROW_LOGIC("tileset has no columns");
        }

        const auto tile_x = static_cast <std::size_t>(id % actual_columns);
        const auto tile_y = static_cast <std::size_t>(id / actual_columns);
        const auto stride_x_size = static_cast <std::size_t>(tile_width) + spacing;
        const auto stride_y_size = static_cast <std::size_t>(tile_height) + spacing;
        const auto margin_size = static_cast <std::size_t>(margin);
        if (tile_x > (std::numeric_limits <std::size_t>::max() - margin_size) / stride_x_size ||
            tile_y > (std::numeric_limits <std::size_t>::max() - margin_size) / stride_y_size) {
            THROW_OUT_OF_RANGE("tileset tile id", id, "is out of range");
        }
        const auto x = margin_size + tile_x * stride_x_size;
        const auto y = margin_size + tile_y * stride_y_size;
        if (x + tile_width > image_width || y + tile_height > image_height ||
            x > static_cast <std::size_t>(std::numeric_limits <int>::max()) ||
            y > static_cast <std::size_t>(std::numeric_limits <int>::max())) {
            THROW_OUT_OF_RANGE("tileset tile id", id, "does not fit the tileset image");
        }
        return {
            static_cast <int>(x),
            static_cast <int>(y),
            static_cast <int>(tile_width),
            static_cast <int>(tile_height)
        };
    }

    tile_drawable world_tileset::drawable(world_local_tile_id id) const {
        tile_drawable result;
        result.origin = point{offset_x, offset_y};

        const world_tile* meta = tile(id);
        result.animated = meta != nullptr && !meta->animation.empty();

        if (meta != nullptr && meta->image) {
            // Collection-of-images: the tile owns its whole image.
            result.image = &*meta->image;
            result.src = rect{
                0, 0,
                static_cast <int>(meta->image->width),
                static_cast <int>(meta->image->height)
            };
        } else {
            // Uniform grid: a sub-rect of the shared tileset image.
            result.image = image ? &*image : nullptr;
            result.src = tile_rect(id); // throws for a missing shared image / bad id
        }
        return result;
    }

    const std::vector <world_tile_animation_frame>*
        world_tileset::animation_of(world_local_tile_id id) const noexcept {
        const world_tile* meta = tile(id);
        if (meta != nullptr && !meta->animation.empty()) {
            return &meta->animation;
        }
        return nullptr;
    }

    bool world_tileset::contains(world_tile_id gid) const noexcept {
        if (gid == 0 || gid < first_gid) {
            return false;
        }
        return tile_count == 0 || gid - first_gid < tile_count;
    }

    world_local_tile_id world_tileset::to_local(world_tile_id gid) const {
        if (gid < first_gid) {
            THROW_OUT_OF_RANGE("gid", gid, "does not belong to tileset with first_gid", first_gid);
        }
        return gid - first_gid;
    }

    world_tile_id world_tileset::to_global(world_local_tile_id id) const noexcept {
        return first_gid + id;
    }

    world_orientation world::orientation() const noexcept {
        return m_orientation;
    }

    void world::set_orientation(world_orientation orientation) noexcept {
        m_orientation = orientation;
    }

    world_render_order world::render_order() const noexcept {
        return m_render_order;
    }

    void world::set_render_order(world_render_order order) noexcept {
        m_render_order = order;
    }

    unsigned world::width() const noexcept {
        return m_width;
    }

    unsigned world::height() const noexcept {
        return m_height;
    }

    void world::set_size(unsigned width, unsigned height) noexcept {
        m_width = width;
        m_height = height;
    }

    unsigned world::tile_width() const noexcept {
        return m_tile_width;
    }

    unsigned world::tile_height() const noexcept {
        return m_tile_height;
    }

    void world::set_tile_size(unsigned width, unsigned height) noexcept {
        m_tile_width = width;
        m_tile_height = height;
    }

    sdlpp::color world::background_color() const noexcept {
        return m_background_color;
    }

    void world::set_background_color(sdlpp::color color) noexcept {
        m_background_color = color;
    }

    unsigned world::hex_side_length() const noexcept {
        return m_hex_side_length;
    }

    void world::set_hex_side_length(unsigned length) noexcept {
        m_hex_side_length = length;
    }

    world_stagger_axis world::stagger_axis() const noexcept {
        return m_stagger_axis;
    }

    void world::set_stagger_axis(world_stagger_axis axis) noexcept {
        m_stagger_axis = axis;
    }

    world_stagger_index world::stagger_index() const noexcept {
        return m_stagger_index;
    }

    void world::set_stagger_index(world_stagger_index index) noexcept {
        m_stagger_index = index;
    }

    bool world::infinite() const noexcept {
        return m_infinite;
    }

    void world::set_infinite(bool infinite) noexcept {
        m_infinite = infinite;
    }

    void world::add_tileset(world_tileset tileset) {
        m_tilesets.emplace_back(std::move(tileset));
    }

    void world::add_layer(world_layer layer) {
        m_layers.emplace_back(std::move(layer));
    }

    const std::vector <world_tileset>& world::tilesets() const noexcept {
        return m_tilesets;
    }

    std::vector <world_tileset>& world::tilesets() noexcept {
        return m_tilesets;
    }

    const std::vector <world_layer>& world::layers() const noexcept {
        return m_layers;
    }

    std::vector <world_layer>& world::layers() noexcept {
        return m_layers;
    }

    const world_tileset* world::tileset_for(world_tile_id gid) const noexcept {
        if (gid == 0) {
            return nullptr;
        }
        const world_tileset* best = nullptr;
        for (const auto& tileset : m_tilesets) {
            if (tileset.first_gid <= gid && (best == nullptr || tileset.first_gid > best->first_gid)) {
                best = &tileset;
            }
        }
        return best;
    }

    std::vector <const world_object_layer*> world::object_layers() const {
        return layers_of <world_object_layer>();
    }

    std::vector <const world_tile_layer*> world::tile_layers() const {
        return layers_of <world_tile_layer>();
    }

    std::vector <const world_image_layer*> world::image_layers() const {
        return layers_of <world_image_layer>();
    }
}
