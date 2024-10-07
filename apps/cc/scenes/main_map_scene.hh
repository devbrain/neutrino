//
// Created by igor on 10/6/24.
//

#ifndef  NEUTRINO_APPS_CC_SCENES_MAIN_MAP_SCENE_HH_
#define  NEUTRINO_APPS_CC_SCENES_MAIN_MAP_SCENE_HH_

#include <neutrino/scene/ecs_world_scene.hh>
#include "level/maps_registry.hh"
#include "level/hud.hh"

class main_map_scene : public neutrino::ecs_world_scene {
 public:
	main_map_scene(const neutrino::sdl::area_type& window_dimensions, const maps_registry& maps);
 protected:
	const neutrino::sdl::area_type& get_window_dimensions() const;
	const maps_registry& get_maps() const;
 protected:
	virtual std::unique_ptr<hud> init_hud(neutrino::sdl::renderer& renderer) = 0;
 private:
	void draw(neutrino::sdl::renderer& renderer) override;
	void init_with_renderer(neutrino::sdl::renderer& renderer) override;
 private:
	neutrino::sdl::area_type m_window_dimensions;
	const maps_registry& m_maps;
	std::unique_ptr<hud> m_hud;
};


#endif
