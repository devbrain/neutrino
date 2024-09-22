//
// Created by igor on 8/28/24.
//

#include <sdlpp/sdlpp.hh>
#include "level_viewer_app.hh"
#include "level_editor_ui.hh"
#include <assets/assets.hh>
#include "data_loader/data_manager.hh"

static tiles_holder generate_fonts(const neutrino::sdl::renderer& renderer) {
    std::ostringstream os;
    for (int i=0; i<256; i++) {
        os << static_cast<char>(i);
    }
    auto srf = neutrino::assets::render_romfont_8x8(os.str(), neutrino::sdl::colors::beige, neutrino::sdl::colors::black);
    std::vector<neutrino::sdl::rect> rects(256);
    for (int x=0; x<256; x++) {
        rects.emplace_back(8*x, 0, 8, 8);
    }
    return tiles_holder(neutrino::sdl::texture (renderer, srf), std::move(rects), 8, 8);
}

level_viewer_app::level_viewer_app(const std::filesystem::path& path_to_data)
    : m_data_dir(path_to_data) {
}

void level_viewer_app::setup_scenes(neutrino::sdl::renderer& renderer) {
    auto [srf, rects] = m_data_dir.load_tileset(data_directory::CC1_TILES);
    m_ctx.m_level_maps = m_data_dir.load_levels(data_directory::CC1_EXE);
    m_ctx.m_tiles_holder = tiles_holder(neutrino::sdl::texture (renderer, srf), std::move(rects), 16, 16);
    m_ctx.m_fonts = generate_fonts(renderer);
    get_scene_manager().push(std::make_shared<level_editor_ui>(m_ctx));
}


