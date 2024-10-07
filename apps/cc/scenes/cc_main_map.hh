//
// Created by igor on 10/6/24.
//

#ifndef  NEUTRINO_APPS_CC_SCENES_CC_MAIN_MAP_HH_
#define  NEUTRINO_APPS_CC_SCENES_CC_MAIN_MAP_HH_

#include "scenes/main_map_scene.hh"

class cc_main_map : public main_map_scene {
 public:
	cc_main_map(const neutrino::sdl::area_type& window_dimensions,
				const maps_registry& maps);
 private:
	void initialize_systems() override;
	void initialize_scene() override;
	void post_initialize() override;

	std::unique_ptr<neutrino::world_renderer> init_world_renderer(neutrino::sdl::renderer& renderer) override;
	std::unique_ptr<hud> init_hud(neutrino::sdl::renderer& renderer) override;
};


#endif
