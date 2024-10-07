//
// Created by igor on 10/6/24.
//

#include "cc_main_map.hh"
#include "level/crystal_caves/crystal_caves_hud.hh"
#include "tile_names.hh"

cc_main_map::cc_main_map(const neutrino::sdl::area_type& window_dimensions, const maps_registry& maps)
	: main_map_scene(window_dimensions, maps) {
}

void cc_main_map::initialize_systems() {

}

void cc_main_map::initialize_scene() {

}

void cc_main_map::post_initialize() {
	ecs_scene::post_initialize();
}

std::unique_ptr<neutrino::world_renderer> cc_main_map::init_world_renderer(neutrino::sdl::renderer& renderer) {
	auto window = get_window_dimensions();
	window.h -= HUD_TILE_H;
	auto wr = std::make_unique<neutrino::world_renderer>(renderer, window);

	return wr;
}

std::unique_ptr<hud> cc_main_map::init_hud(neutrino::sdl::renderer& renderer) {
	return std::make_unique<crystal_caves_hud>(get_window_dimensions().h - HUD_TILE_H);
}
