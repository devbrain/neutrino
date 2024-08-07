//
// Created by igor on 8/7/24.
//

#include <neutrino/modules/video/systems/sprite_system.hh>
#include <neutrino/modules/physics/components/body.hh>

namespace neutrino {
    void sprite_system::update(ecs::registry& registry, std::chrono::milliseconds delta_t, const sdl::rect& viewport) {
        registry.iterate([this, delta_t]([[maybe_unused]] ecs::entity_id_t eid, animated_sprite& s) {
            _update(s, delta_t);
        });
        registry.iterate([this, delta_t]([[maybe_unused]] ecs::entity_id_t eid, animated_sprite_sequence& s) {
            _update(s.states[s.current_state], delta_t);
        });
    }

    void sprite_system::present(ecs::registry& registry, sdl::renderer& renderer, const sdl::rect& viewport,
                                const texture_atlas& atlas) {
        registry.iterate([this, &renderer, &atlas]([[maybe_unused]] ecs::entity_id_t eid, const single_tile_sprite& s, const body& b) {
            _present(s.sprite, renderer, b.position.x, b.position.y, atlas);
        });
        registry.iterate([this, &renderer, &atlas]([[maybe_unused]] ecs::entity_id_t eid, const animated_sprite& s, const body& b) {
            _present(s, renderer, b.position.x, b.position.y, atlas);
        });

        registry.iterate(
            [this, &renderer, &atlas]([[maybe_unused]] ecs::entity_id_t eid, const animated_sprite_sequence& s, const body& b) {
                _present(s.states[s.current_state], renderer, b.position.x, b.position.y, atlas);
            });
    }

    void sprite_system::_update(animated_sprite& sprite, std::chrono::milliseconds delta_t) {
        sprite.time_in_state += delta_t;
        if (sprite.time_in_state > sprite.sequence.get_frames()[sprite.current_frame].m_duration) {
            sprite.time_in_state = std::chrono::milliseconds(0);
            if (sprite.current_frame < sprite.sequence.get_frames().size() - 1) {
                sprite.current_frame++;
            } else {
                sprite.current_frame = 0;
            }
        }
    }

    void sprite_system::_present(const animated_sprite& sprite, sdl::renderer& r, int x, int y,
                                 const texture_atlas& atlas) {
        _present(sprite.sequence.get_frames()[sprite.current_frame].m_tile, r, x, y, atlas);
    }

    void sprite_system::_present(const tile& sprite, sdl::renderer& r, int x, int y, const texture_atlas& atlas) {
        auto [text_ptr, rect] = atlas.get(sprite);
        if (text_ptr) {
            const sdl::rect dst_rect(x, y, rect.w, rect.h);
            r.copy(*text_ptr, rect, dst_rect);
        }
    }
}
