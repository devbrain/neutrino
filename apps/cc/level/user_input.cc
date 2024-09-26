//
// Created by igor on 9/23/24.
//

#include "user_input.hh"

user_input::user_input()
	: m_key_move_left(neutrino::sdl::scancode::LEFT),
	  m_key_move_right(neutrino::sdl::scancode::RIGHT),
	  m_key_jump(neutrino::sdl::scancode::LALT),
	  m_key_fire(neutrino::sdl::scancode::LCTRL),
	  m_reactor(nullptr) {
}

const neutrino::config_hotkey& user_input::get_key_move_left() const {
	return m_key_move_left;
}

const neutrino::config_hotkey& user_input::get_key_move_right() const {
	return m_key_move_right;
}

const neutrino::config_hotkey& user_input::get_key_jump() const {
	return m_key_jump;
}

const neutrino::config_hotkey& user_input::get_key_fire() const {
	return m_key_fire;
}

void user_input::register_in_reactor(neutrino::events_reactor& reactor) {
	m_reactor = &reactor;
	m_mapper.register_hotkey(m_key_move_left, pressed_key::MOVE_LEFT);
	m_mapper.register_hotkey(m_key_move_right, pressed_key::MOVE_RIGHT);
	m_mapper.register_hotkey(m_key_jump, pressed_key::JUMP);
	m_mapper.register_hotkey(m_key_fire, pressed_key::FIRE);
	m_mapper.add_to_reactor(reactor);
}

bool user_input::check(user_input::pressed_key k) const {
	switch (k) {
		case NONE:
			return false;
		case MOVE_LEFT:
			return neutrino::hotkey_mapper::is_pressed(m_key_move_left);
		case MOVE_RIGHT:
			return neutrino::hotkey_mapper::is_pressed(m_key_move_right);
		case JUMP:
			return neutrino::hotkey_mapper::is_pressed(m_key_jump);
		case FIRE:
			return neutrino::hotkey_mapper::is_pressed(m_key_fire);
	}
	return false;
}

