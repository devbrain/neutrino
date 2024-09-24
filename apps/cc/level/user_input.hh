//
// Created by igor on 9/23/24.
//

#ifndef  NEUTRINO_APPS_CC_LEVEL_USER_INPUT_HH_
#define  NEUTRINO_APPS_CC_LEVEL_USER_INPUT_HH_

#include <neutrino/config/config_hotkey.hh>
#include <neutrino/events/events_reactor.hh>
#include <neutrino/events/hotkey_mapper.hh>

class user_input {
 public:
	enum pressed_key : int {
		NONE = 0,
		MOVE_LEFT = 1,
		MOVE_RIGHT = 2,
		JUMP = 3,
		FIRE = 4
	};
	user_input();

	void register_in_reactor(neutrino::events_reactor& reactor);

	[[nodiscard]] const neutrino::config_hotkey& get_key_move_left() const;
	[[nodiscard]] const neutrino::config_hotkey& get_key_move_right() const;
	[[nodiscard]] const neutrino::config_hotkey& get_key_jump() const;
	[[nodiscard]] const neutrino::config_hotkey& get_key_fire() const;

	[[nodiscard]] pressed_key get_key() const;
	[[nodiscard]] bool check_key(pressed_key k) const;
 private:
	neutrino::config_hotkey m_key_move_left;
	neutrino::config_hotkey m_key_move_right;
	neutrino::config_hotkey m_key_jump;
	neutrino::config_hotkey m_key_fire;
	neutrino::events_reactor* m_reactor;

	neutrino::hotkey_mapper m_mapper;

};






#endif
