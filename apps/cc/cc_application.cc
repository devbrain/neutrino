//
// Created by igor on 8/28/24.
//


#include "data_loader/data_directory.hh"
#include "level/crystal_caves/crystal_caves_map.hh"
#include "scenes/title_scene.hh"
#include "scenes/main_screen_scene.hh"
#include "scenes/scenes_registry.hh"
#include "scenes/main_level_scene.hh"

#include "cc_application.hh"


neutrino::tiled::world_model get_map(int name, neutrino::ecs::registry& reg) {
    return dynamic_cast <const cc_application&>(neutrino::application::instance()).get_map(name, reg);
}

cc_application::cc_application(const std::filesystem::path& path_to_data)
    : m_factory(factory::create(game_name::CC1, path_to_data)) {
}

neutrino::tiled::world_model cc_application::get_map(int name, neutrino::ecs::registry& reg) const {
    return m_maps_registry->get_map(name, reg);
}

void cc_application::load_maps() {
    auto [levels, text] = m_factory->load_levels_and_text();
    m_maps_registry = std::make_unique <crystal_caves_map>(levels);
    m_dialogs_factory = std::make_unique<dialogs_factory>(m_factory->get_game_name(), std::move(text));
}

void cc_application::setup_scenes(neutrino::sdl::renderer& renderer) {
    load_maps();
    load_tiles(renderer);
    auto title = create_title_scene(renderer);
    auto main_level = create_main_level(renderer);
    scenes_registry::instance().add(scene_name_t::TITLE_SCREEN, title);
    scenes_registry::instance().add(scene_name_t::MAIN_SCREEN, create_main_scene(renderer));
    m_dialogs_factory->create_dialogs(renderer);

    get_scene_manager().push(main_level);
}

void cc_application::load_tiles(const neutrino::sdl::renderer& renderer) {
    m_factory->setup_tiles(get_texture_atlas(), renderer);
}

std::shared_ptr <neutrino::scene> cc_application::create_main_scene(const neutrino::sdl::renderer& renderer) {
    auto [title_up_id, title_down_id] = m_factory->load_title(get_texture_atlas(), renderer);
    return std::make_shared <main_screen_scene>(title_up_id, title_down_id, 25, std::chrono::seconds(3));
}

std::shared_ptr <neutrino::scene> cc_application::create_title_scene(const neutrino::sdl::renderer& renderer) {
    auto title_id = m_factory->load_publisher(get_texture_atlas(), renderer);
    return std::make_shared <title_scene>(title_id, std::chrono::seconds(5));
}

std::shared_ptr <neutrino::scene> cc_application::create_main_level(neutrino::sdl::renderer& renderer) const {
    auto window_dimensions = this->get_window_dimensions();
    return std::make_shared <main_level_scene>(renderer, window_dimensions, *m_maps_registry);
}
