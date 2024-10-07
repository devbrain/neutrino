//
// Created by igor on 8/7/24.
//

#include <neutrino/modules/video/systems/sprite_system.hh>
#include <neutrino/modules/physics/components/body.hh>
#include <neutrino/modules/video/components/world_renderer_camera.hh>

namespace neutrino::ecs::video {

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
		auto itr = registry.iterator<world_renderer_camera>();
		while (itr.has_next()) {
			const auto [_, vw] = itr.next();
			sdl::rect viewport (vw->camera_position, vw->view_port);

			registry.iterate([this, viewport]([[maybe_unused]] entity_id_t eid,
															const single_tile_sprite& s,
															const physics::body& b) {
				this->draw_tile(s.sprite, b.position, viewport);
			});
			registry.iterate([this, &viewport]([[maybe_unused]] entity_id_t eid,
															const animated_sprite& s,
															const physics::body& b) {
			  this->draw_tile(s.sequence, s.current_frame, b.position, viewport);
			});
			registry.iterate(
				[this, &viewport]([[maybe_unused]] entity_id_t eid,
											   const animated_sprite_sequence& s,
											   const physics::body& b) {
					this->draw_tile(s.states[s.current_state].sequence,
									s.states[s.current_state].current_frame, b.position, viewport);
				});

			registry.iterate(
				[this, &viewport]([[maybe_unused]] entity_id_t eid,
											   const sprite_bank& s,
											   const physics::body& b) {
				  this->draw_tile(s.sprites[s.current], b.position, viewport);
				});

			registry.iterate(
				[this, &viewport]([[maybe_unused]] entity_id_t eid,
											   const sprite_bank_array& s,
											   const physics::body& b) {
				  const auto& bank = s.banks[s.current];
				  this->draw_tile(bank.sprites[bank.current], b.position, viewport);
				});
			break;
		}
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

}
