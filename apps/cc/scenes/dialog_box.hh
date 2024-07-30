//
// Created by igor on 7/21/24.
//

#ifndef  DIALOG_BOX_HH
#define  DIALOG_BOX_HH

#include <string>
#include <functional>
#include <map>
#include <neutrino/scene/scene.hh>
#include <neutrino/systems/video/world/world_model.hh>
#include <neutrino/systems/video/world_renderer.hh>
#include "scenes/scenes_registry.hh"
#include "neutrino/ecs/registry.hh"


class dialog_box : public neutrino::scene {
	public:

	using scene::pop_scene;

	static void push_scene(scene_name_t name);
	static void replace_scene(scene_name_t name);

	using keys_map_t = std::map<neutrino::sdl::scancode, std::function<void ()>>;
	public:
		dialog_box(neutrino::sdl::renderer& r, const std::string& text, keys_map_t key_mapping);

	private:
		void update(std::chrono::milliseconds delta_time) override;
		void render(neutrino::sdl::renderer& renderer) override;
		void initialize() override;
		flags get_flags() const override;

		neutrino::world_renderer m_world_renderer;
		neutrino::tiled::world_model m_world_model;
		keys_map_t m_key_mapping;
		neutrino::ecs::registry m_stars;
};



#endif
