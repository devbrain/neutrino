//
// Created by igor on 8/28/24.
//

#include <sdlpp/sdlpp.hh>
#include "props_editor_app.hh"
#include "props_editor_ui.hh"

props_editor_app::props_editor_app(const std::filesystem::path& path_to_data)
    : m_data_dir(path_to_data) {
}

void props_editor_app::setup_scenes(neutrino::sdl::renderer& renderer) {
    auto [srf, rects] = m_data_dir.load_tileset(data_directory::CC1_TILES);

    get_scene_manager().push(std::make_shared<props_editor_ui>(neutrino::sdl::texture (renderer, srf), std::move(rects)));
}


