//
// Created by igor on 9/23/24.
//

#include "level/level.hh"
#include <bsw/magic_enum/magic_enum.hpp>
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

void level::update(std::chrono::milliseconds delta_time) {
	auto k = s_user_input.get_key();
	if (k != user_input::NONE) {
		EVLOG_TRACE(EVLOG_INFO, "Key pressed ", magic_enum::enum_name(k));
	}
}


