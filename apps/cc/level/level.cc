//
// Created by igor on 9/23/24.
//

#include "level/level.hh"
#include "level/player_flags.hh"
#include <neutrino/modules/physics/components/body.hh>
#include <neutrino/modules/video/components/sprite_component.hh>
#include "level/crystal_caves/crystal_caves_sprite_states.hh"
#include <bsw/logger/logger.hh>


user_input level::s_user_input {};

neutrino::tiled::world_model& level::get_model() {
	return m_model;
}

const neutrino::tiled::world_model& level::get_model() const {
	return m_model;
}

ecs_registry& level::get_registry() {
	return m_registry;
}

const ecs_registry& level::get_registry() const {
	return m_registry;
}

user_input& level::get_user_input_handler() {
	return s_user_input;
}

void level::update(std::chrono::milliseconds delta_time, neutrino::sdl::rect& viewport) {
	process_player_actions();
	move_game_objects(delta_time); // bring the game to the current state
	update_game_state(viewport);
}

void level::move_game_objects(std::chrono::milliseconds delta_time) {

}

void level::process_player_actions() {
	const auto jump_pressed = s_user_input.check(user_input::JUMP);
	const auto fire_pressed = s_user_input.check(user_input::FIRE);
	const auto left_pressed = s_user_input.check(user_input::MOVE_LEFT);
	const auto right_pressed = s_user_input.check(user_input::MOVE_RIGHT);

	const float speed = 74.0f; // pixel per second
	const float speed_ms = speed / 1000.0f;

	auto& player_body = m_registry.get_component<neutrino::body>();

	if (right_pressed) {
		player_body.speed.x = speed_ms;
		player_body.set_flag(PLAYER_FLAGS_MOVING_RIGHT);
	} else {
		player_body.clear_flag(PLAYER_FLAGS_MOVING_RIGHT);
		player_body.speed.x = 0;
	}

	if (left_pressed) {
		player_body.speed.x = -speed_ms;
		player_body.set_flag(PLAYER_FLAGS_MOVING_LEFT);
	} else {
		player_body.clear_flag(PLAYER_FLAGS_MOVING_LEFT);
		if (!player_body.has_flag(PLAYER_FLAGS_MOVING_RIGHT)) {
			player_body.speed.x = 0;
		}
	}


}

void level::update_game_state(neutrino::sdl::rect& viewport) {
	auto& player_body = m_registry.get_component<neutrino::body>();
	auto& mylo_sprite = m_registry.get_component<neutrino::sprite_bank_array>();
	static constexpr std::chrono::milliseconds time_in_frame{200};
	if (player_body.has_flag(PLAYER_FLAGS_MOVING_RIGHT)) {
		mylo_sprite.set_current(SPRITE_STATE_MOVE_RIGHT);
		mylo_sprite.next(time_in_frame);
	} else if (player_body.has_flag(PLAYER_FLAGS_MOVING_LEFT)) {
		mylo_sprite.set_current(SPRITE_STATE_MOVE_LEFT);
		mylo_sprite.next(time_in_frame);
	}
	update_game_camera(viewport);
}

void level::update_game_camera(neutrino::sdl::rect& viewport) {
	auto& player_body = m_registry.get_component<neutrino::body>();
	neutrino::sdl::point player = player_body.position - viewport.center();
	auto offs = viewport.offset();
	if (player_body.has_flag(PLAYER_FLAGS_MOVING_RIGHT)) {
		if (player.x > 0) {
			offs.x += player.x;
		}
		if (player.y > 0) {
			offs.y += player.y;
		}
	}

	if (player_body.has_flag(PLAYER_FLAGS_MOVING_LEFT)) {
		if (player.x < 0) {
			offs.x += player.x;
		}
		if (player.y < 0) {
			offs.y += player.y;
		}
	}
	viewport.offset(offs);
}
