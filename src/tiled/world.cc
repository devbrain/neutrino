//
// Created by igor on 06/07/2021.
//

#include <neutrino/tiled/world.hh>

namespace neutrino::tiled {
    std::size_t world::add(tiles_layer world_layer) {
        auto n = m_layers.size();
        m_layers.emplace_back(world_layer);
        return n;
    }
    // -----------------------------------------------------------------------------------------
    std::size_t world::add(image_layer world_layer) {
        auto n = m_layers.size();
        m_layers.emplace_back(world_layer);
        return n;
    }
    // -----------------------------------------------------------------------------------------
    void world::clear() {
        m_layers.clear();
        m_sprites.clear();
        m_bound_sprites.clear();
    }
    // -----------------------------------------------------------------------------------------
    std::size_t world::add(sprite sprite_frames)
    {
        auto n = m_sprites.size();
        m_sprites.push_back(std::move(sprite_frames));
        return n;
    }
    // -----------------------------------------------------------------------------------------
    std::size_t world::bind_sprite_to_layer(std::size_t sprite_id, std::size_t layer_id, std::size_t current_frame, bool active)
    {
        auto n = m_bound_sprites.size();
        m_bound_sprites.push_back({sprite_id, current_frame, active});
        m_sprites_to_layers[layer_id].push_back(n);
        return n;
    }
    // -----------------------------------------------------------------------------------------
    std::size_t world::bind_sprite_to_layer(std::size_t sprite_id, std::size_t layer_id, std::size_t current_frame)
    {
        return bind_sprite_to_layer(sprite_id, layer_id, current_frame, true);
    }
    // -----------------------------------------------------------------------------------------
    std::size_t world::bind_sprite_to_layer(std::size_t sprite_id, std::size_t layer_id)
    {
        return bind_sprite_to_layer(sprite_id, layer_id, 0, true);
    }
    // -----------------------------------------------------------------------------------------
    void world::sprite_state(std::size_t bound_sprite_id, std::size_t current_frame, bool active)
    {
        m_bound_sprites[bound_sprite_id].active = active;
        m_bound_sprites[bound_sprite_id].current = current_frame;
    }
    // -----------------------------------------------------------------------------------------
    std::tuple<std::size_t, bool> world::sprite_state(std::size_t bound_sprite_id) const
    {
        return {m_bound_sprites[bound_sprite_id].current, m_bound_sprites[bound_sprite_id].active};
    }
}