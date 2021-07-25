//
// Created by igor on 06/07/2021.
//

#include <neutrino/tiled/world/world.hh>
#include <neutrino/utils/override.hh>

namespace neutrino::tiled {
    layer_id_t world::add(tiles_layer world_layer) {
        auto n = m_layers.size();
        m_layers.emplace_back(world_layer);
        return layer_id_t {n};
    }
    // -----------------------------------------------------------------------------------------
    layer_id_t world::add(image_layer world_layer) {
        auto n = m_layers.size();
        m_layers.emplace_back(world_layer);
        return layer_id_t {n};
    }
    // -----------------------------------------------------------------------------------------
    void world::clear() {
        m_layers.clear();
        m_sprites.clear();
        m_bound_sprites.clear();
    }
    // -----------------------------------------------------------------------------------------
    sprite_id_t world::add(sprite sprite_frames)
    {
        auto n = m_sprites.size();
        m_sprites.push_back(std::move(sprite_frames));
        return sprite_id_t {n};
    }
    // -----------------------------------------------------------------------------------------
    bound_sprite_id_t world::bind_sprite_to_layer(sprite_id_t sprite_id, layer_id_t layer_id, std::size_t current_frame, bool active)
    {
        bound_sprite_id_t n {m_bound_sprites.size()};
        m_bound_sprites.push_back({sprite_id, current_frame, active});
        m_sprites_to_layers[layer_id].push_back(n);
        return n;
    }
    // -----------------------------------------------------------------------------------------
    bound_sprite_id_t world::bind_sprite_to_layer(sprite_id_t sprite_id, layer_id_t layer_id, std::size_t current_frame)
    {
        return bind_sprite_to_layer(sprite_id, layer_id, current_frame, true);
    }
    // -----------------------------------------------------------------------------------------
    bound_sprite_id_t world::bind_sprite_to_layer(sprite_id_t sprite_id, layer_id_t layer_id)
    {
        return bind_sprite_to_layer(sprite_id, layer_id, 0, true);
    }
    // -----------------------------------------------------------------------------------------
    void world::sprite_state(bound_sprite_id_t bound_sprite_id, std::size_t current_frame, bool active)
    {
        m_bound_sprites[value_of(bound_sprite_id)].active = active;
        m_bound_sprites[value_of(bound_sprite_id)].current = current_frame;
    }
    // -----------------------------------------------------------------------------------------
    std::tuple<std::size_t, std::size_t, bool> world::sprite_state(bound_sprite_id_t bound_sprite_id) const
    {
        const auto n = value_of(bound_sprite_id);
        return {m_bound_sprites[n].current,
                m_sprites[value_of(m_bound_sprites[n].sprite_id)].frames.size(),
                m_bound_sprites[n].active};
    }
    // -----------------------------------------------------------------------------------------
    math::dimension_t world::dims_in_pixels(layer_id_t layer_id) const {
        const auto& layer = m_layers[value_of(layer_id)];
        math::dimension_t ret;
        std::visit(neutrino::utils::overload(
                [&ret] (const auto& tl) {
                    ret = {tl.w*tl.tile_width, tl.h*tl.tile_height};
                }), layer);
        return ret;
    }
    // -----------------------------------------------------------------------------------------
    math::dimension_t world::dims_in_tiles(layer_id_t layer_id) const {
        const auto& layer = m_layers[value_of(layer_id)];
        math::dimension_t ret;
        std::visit(neutrino::utils::overload(
                [&ret] (const auto& tl) {
                    ret = {tl.w, tl.h};
                }), layer);
        return ret;
    }
    // -----------------------------------------------------------------------------------------
    math::dimension_t world::tile_dims(layer_id_t layer_id) const {
        const auto& layer = m_layers[value_of(layer_id)];
        math::dimension_t ret;
        std::visit(neutrino::utils::overload(
                [&ret] (const auto & tl) {
                    ret = {tl.tile_width, tl.tile_height};
                }), layer);
        return ret;
    }
}