//
// Created by igor on 8/25/24.
//

#include "main_level_scene.hh"
#include "tile_names.hh"
#include "neutrino/application.hh"

static auto init_world_renderer(neutrino::sdl::renderer& r, const neutrino::sdl::area_type& window_size) {
    auto window = window_size;
    window.h -= HUD_TILE_H;
    return neutrino::world_renderer(r, window);
}

main_level_scene::main_level_scene(neutrino::sdl::renderer& r, const neutrino::sdl::area_type& window_size, const maps_registry& maps)
    : m_world_renderer(init_world_renderer(r, window_size)),
      m_hud(static_cast <int>(window_size.h - HUD_TILE_H)) {
      m_world = maps.get_map(maps_registry::MAIN_LEVEL, m_world_renderer);
//    neutrino::serialize_tmx("zopa-tmx", m_world_model, neutrino::application::instance().get_texture_atlas(), r);
}

void main_level_scene::update(std::chrono::milliseconds delta_time) {
	m_world->update(delta_time);
    m_world_renderer.update(delta_time);
}

void main_level_scene::render(neutrino::sdl::renderer& renderer) {
    m_world_renderer.present();
    m_hud.draw(renderer, get_texture_atlas());
}

void main_level_scene::initialize() {
    m_world_renderer.init(get_texture_atlas(), m_world->get_model());
	level::get_user_input_handler().register_in_reactor(get_events_reactor());
}
