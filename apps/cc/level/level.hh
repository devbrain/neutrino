//
// Created by igor on 9/23/24.
//

#ifndef NEUTRINO_APPS_CC_LEVEL_LEVEL_HH_
#define NEUTRINO_APPS_CC_LEVEL_LEVEL_HH_

#include <chrono>

#include <neutrino/modules/video/world/world_model.hh>
#include "level/ecs_registry.hh"
#include "level/user_input.hh"

class level {
 public:
	level() = default;

	neutrino::tiled::world_model& get_model ();
	[[nodiscard]] const neutrino::tiled::world_model& get_model () const;

	ecs_registry& get_registry();
	[[nodiscard]] const ecs_registry& get_registry() const;

	void update(std::chrono::milliseconds delta_time);

	static user_input& get_user_input_handler();
 private:
	neutrino::tiled::world_model m_model;
	ecs_registry m_registry;
	static user_input s_user_input;
};

#endif
