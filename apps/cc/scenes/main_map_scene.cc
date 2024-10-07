//
// Created by igor on 10/6/24.
//

#include "main_map_scene.hh"

main_map_scene::main_map_scene(const neutrino::sdl::area_type& window_dimensions, const maps_registry& maps)
: m_window_dimensions(window_dimensions), m_maps(maps){

}

void main_map_scene::draw(neutrino::sdl::renderer& renderer) {
	if (m_hud) {
		m_hud->draw(renderer, get_texture_atlas());
	}
}


const neutrino::sdl::area_type& main_map_scene::get_window_dimensions() const {
	return m_window_dimensions;
}

const maps_registry& main_map_scene::get_maps() const {
	return m_maps;
}

void main_map_scene::init_with_renderer(neutrino::sdl::renderer& renderer) {
	m_hud = init_hud(renderer);
}


