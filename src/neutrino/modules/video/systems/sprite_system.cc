//
// Created by igor on 8/7/24.
//

#include <neutrino/modules/video/systems/sprite_system.hh>
#include <neutrino/modules/physics/components/body.hh>

namespace neutrino::ecs::video {
	sprite_system::sprite_system(const world_renderer& world_renderer_)
		: world_rendering_system(world_renderer_) {
	}

	void sprite_system::update(registry& registry, std::chrono::milliseconds delta_t) {
		registry.iterate([delta_t]([[maybe_unused]] entity_id_t eid, animated_sprite& s) {
		  _update(s, delta_t);
		});
		registry.iterate([delta_t]([[maybe_unused]] entity_id_t eid, animated_sprite_sequence& s) {
		  _update(s.states[s.current_state], delta_t);
		});
		registry.iterate([delta_t]([[maybe_unused]] entity_id_t eid, sprite_bank& s) {
		  _update(s, delta_t);
		});
		registry.iterate([delta_t]([[maybe_unused]] entity_id_t eid, sprite_bank_array& s) {
		  _update(s.banks[s.current], delta_t);
		});
	}

	void sprite_system::present(registry& registry) {
		auto& renderer = get_renderer();
		const auto& atlas = get_atlas();
		const auto viewport = get_world_viewport();
		registry.iterate([&renderer, &atlas, &viewport]([[maybe_unused]] entity_id_t eid,
														const single_tile_sprite& s,
														const physics::body& b) {
		  _present(s.sprite, renderer, b.position, viewport, atlas);
		});
		registry.iterate([&renderer, &atlas, &viewport]([[maybe_unused]] entity_id_t eid,
														const animated_sprite& s,
														const physics::body& b) {
		  _present(s, renderer, b.position, viewport, atlas);
		});
		registry.iterate(
			[&renderer, &atlas, &viewport]([[maybe_unused]] entity_id_t eid,
										   const animated_sprite_sequence& s,
										   const physics::body& b) {
			  _present(s.states[s.current_state], renderer, b.position, viewport, atlas);
			});

		registry.iterate(
			[&renderer, &atlas, &viewport]([[maybe_unused]] entity_id_t eid,
										   const sprite_bank& s,
										   const physics::body& b) {
			  _present(s.sprites[s.current], renderer, b.position, viewport, atlas);
			});

		registry.iterate(
			[&renderer, &atlas, &viewport]([[maybe_unused]] entity_id_t eid,
										   const sprite_bank_array& s,
										   const physics::body& b) {
			  const auto& bank = s.banks[s.current];
			  _present(bank.sprites[bank.current], renderer, b.position, viewport, atlas);
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

	void sprite_system::_update(sprite_bank& sprite, std::chrono::milliseconds delta_t) {
		sprite.time_in_current_frame += delta_t;
	}

	void sprite_system::_present(const animated_sprite& sprite,
								 sdl::renderer& r,
								 const sdl::point& pos,
								 const sdl::rect& viewport,
								 const texture_atlas& atlas) {
		_present(sprite.sequence.get_frames()[sprite.current_frame].m_tile, r, pos, viewport, atlas);
	}

	void sprite_system::_present(const tile& sprite,
								 sdl::renderer& r,
								 const sdl::point& pos,
								 const sdl::rect& viewport,
								 const texture_atlas& atlas) {
		auto [text_ptr, rect] = atlas.get(sprite);
		if (text_ptr) {
			sdl::rect dst_rect(pos.x, pos.y, rect.w, rect.h);
			auto ir = dst_rect.intersection(viewport);
			if (ir) {
				dst_rect = *ir;
				dst_rect.x = dst_rect.x - viewport.x;
				dst_rect.y = dst_rect.y - viewport.y;
				r.copy(*text_ptr, rect, dst_rect);
			}
		}
	}


}
