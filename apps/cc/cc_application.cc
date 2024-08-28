//
// Created by igor on 8/28/24.
//

#include <neutrino/scene/title_scene.hh>
#include "data_loader/data_directory.hh"
#include "data_loader/data_manager.hh"
#include "tile_names.hh"
#include "scenes/title_scene.hh"
#include "scenes/main_screen_scene.hh"
#include "scenes/scenes_registry.hh"
#include "scenes/dialogs.hh"
#include "scenes/main_level_scene.hh"
#include "cc_application.hh"

neutrino::tiled::world_model get_map(int name, neutrino::ecs::registry& reg) {
    return dynamic_cast <const cc_application&>(neutrino::application::instance()).get_map(name, reg);
}

cc_application::cc_application(const std::filesystem::path& path_to_data)
    : m_data_dir(path_to_data) {
}

neutrino::tiled::world_model cc_application::get_map(int name, neutrino::ecs::registry& reg) const {
    return m_maps_registry->get_map(name, reg);
}

void cc_application::load_maps() {
    m_maps_registry = std::make_unique <maps_registry>(m_data_dir.load_maps(data_directory::CC1_EXE));
}

void cc_application::setup_scenes(neutrino::sdl::renderer& renderer) {
    load_maps();
    load_tiles(renderer);
    auto title = create_title_scene(renderer);
    auto main_level = create_main_level(renderer);
    scenes_registry::instance().add(scene_name_t::TITLE_SCREEN, title);
    scenes_registry::instance().add(scene_name_t::MAIN_SCREEN, create_main_scene(renderer));
    scenes_registry::instance().add(scene_name_t::MAIN_DIALOG, create_main_dialog_box(renderer));
    scenes_registry::instance().add(scene_name_t::QUIT_TO_DOS_DIALOG, create_quit_to_dos_dialog_box(renderer));
    get_scene_manager().push(main_level);
}

void cc_application::load_tiles(const neutrino::sdl::renderer& renderer) {
    auto cc_mini_tiles_1 = get_texture_atlas().add(m_data_dir.load_tileset(data_directory::CC1_MINI_TILES1), renderer);
    auto cc_mini_tiles_2 = get_texture_atlas().add(m_data_dir.load_tileset(data_directory::CC1_MINI_TILES2), renderer);
    auto cc_mini_tiles_3 = get_texture_atlas().add(m_data_dir.load_tileset(data_directory::CC1_MINI_TILES3), renderer);
    auto cc_tiles = get_texture_atlas().add(m_data_dir.load_tileset(data_directory::CC1_TILES), renderer);
    setup_cc_names(get_texture_atlas(), cc_mini_tiles_1, cc_mini_tiles_2, cc_mini_tiles_3, cc_tiles);
}

std::shared_ptr <neutrino::scene> cc_application::create_main_scene(neutrino::sdl::renderer& renderer) {
    auto title_up_id = get_texture_atlas().add(m_data_dir.load_picture(data_directory::CC1_UP_MAIN), renderer);
    auto title_down_id = get_texture_atlas().add(m_data_dir.load_picture(data_directory::CC1_DOWN_MAIN), renderer);
    return std::make_shared <main_screen_scene>(title_up_id, title_down_id, 25, std::chrono::seconds(3));
}

std::shared_ptr <neutrino::scene> cc_application::create_title_scene(neutrino::sdl::renderer& renderer) {
    auto title_id = get_texture_atlas().add(m_data_dir.load_picture(data_directory::CC1_APOGEE_SCREEN), renderer);
    return std::make_shared <title_scene>(title_id, std::chrono::seconds(5));
}

std::shared_ptr <neutrino::scene> cc_application::create_main_level(neutrino::sdl::renderer& renderer) {
    auto window_dimensions = this->get_window_dimensions();
    return std::make_shared <main_level_scene>(renderer, window_dimensions, *m_maps_registry);
}