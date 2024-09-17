//
// Created by igor on 9/17/24.
//

#include "factory.hh"
#include <bsw/exception.hh>
#include "tile_names.hh"

std::unique_ptr <factory> factory::create(game_name name, const std::filesystem::path& path_to_data) {
    return std::unique_ptr <factory>{new factory(name, path_to_data)};
}

game_name factory::get_game_name() const {
    return m_game_name;
}

std::tuple <neutrino::texture_id_t, neutrino::texture_id_t> factory::load_title(neutrino::texture_atlas& atlas,
    const neutrino::sdl::renderer& renderer) const {
    auto up_id = data_directory::CC1_UP_MAIN;
    auto down_id = data_directory::CC1_DOWN_MAIN;

    switch (m_game_name) {
        case game_name::CC1:
            up_id = data_directory::CC1_UP_MAIN;
            down_id = data_directory::CC1_DOWN_MAIN;
            break;
        case game_name::CC2:
            up_id = data_directory::CC2_UP_MAIN;
            down_id = data_directory::CC2_DOWN_MAIN;
            break;
        case game_name::CC3:
            up_id = data_directory::CC3_UP_MAIN;
            down_id = data_directory::CC3_DOWN_MAIN;
            break;
        default:
            RAISE_EX("Not implemented yet");
    }

    auto title_up_id = atlas.add(m_data_directory.load_picture(up_id), renderer);
    auto title_down_id = atlas.add(m_data_directory.load_picture(down_id), renderer);
    return {title_up_id, title_down_id};
}

neutrino::texture_id_t factory::load_publisher(neutrino::texture_atlas& atlas,
                                               const neutrino::sdl::renderer& renderer) const {
    auto publisher_id = data_directory::CC1_APOGEE_SCREEN;
    switch (m_game_name) {
        case game_name::CC1:
            publisher_id = data_directory::CC1_APOGEE_SCREEN;
            break;
        case game_name::CC2:
            publisher_id = data_directory::CC2_APOGEE_SCREEN;
            break;
        case game_name::CC3:
            publisher_id = data_directory::CC3_APOGEE_SCREEN;
            break;
        default:
            RAISE_EX("Not implemented yet");
    }
    return atlas.add(m_data_directory.load_picture(publisher_id), renderer);
}

void factory::setup_tiles(neutrino::texture_atlas& atlas,
                          const neutrino::sdl::renderer& renderer) const {
    bool is_cc = false;
    auto mini_tiles_1 = data_directory::CC1_MINI_TILES1;
    auto mini_tiles_2 = data_directory::CC1_MINI_TILES2;
    auto mini_tiles_3 = data_directory::CC1_MINI_TILES3;
    auto tiles = data_directory::CC1_TILES;
    switch (m_game_name) {
        case game_name::CC1:
            is_cc = true;
            mini_tiles_1 = data_directory::CC1_MINI_TILES1;
            mini_tiles_2 = data_directory::CC1_MINI_TILES2;
            mini_tiles_3 = data_directory::CC1_MINI_TILES3;
            tiles = data_directory::CC1_TILES;
            break;
        case game_name::CC2:
            is_cc = true;
            mini_tiles_1 = data_directory::CC2_MINI_TILES1;
            mini_tiles_2 = data_directory::CC2_MINI_TILES2;
            mini_tiles_3 = data_directory::CC2_MINI_TILES3;
            tiles = data_directory::CC2_TILES;
            break;
        case game_name::CC3:
            is_cc = true;
            mini_tiles_1 = data_directory::CC3_MINI_TILES1;
            mini_tiles_2 = data_directory::CC3_MINI_TILES2;
            mini_tiles_3 = data_directory::CC3_MINI_TILES3;
            tiles = data_directory::CC3_TILES;
            break;
        default:
            RAISE_EX("Not implemented yet");
    }
    auto tex_mini_tiles_1 = atlas.add(m_data_directory.load_tileset(mini_tiles_1), renderer);
    auto tex_mini_tiles_2 = atlas.add(m_data_directory.load_tileset(mini_tiles_2), renderer);
    auto tex_mini_tiles_3 = atlas.add(m_data_directory.load_tileset(mini_tiles_3), renderer);
    auto tex_tiles = atlas.add(m_data_directory.load_tileset(tiles), renderer);
    if (is_cc) {
        setup_cc_names(atlas, tex_mini_tiles_1, tex_mini_tiles_2, tex_mini_tiles_3, tex_tiles);
    } else {
        RAISE_EX("Not implemented yet");
    }
}

std::vector<raw_level_map> factory::load_levels() const {
    switch (m_game_name) {
        case game_name::CC1:
            return m_data_directory.load_levels(data_directory::CC1_EXE);
        case game_name::CC2:
            return m_data_directory.load_levels(data_directory::CC2_EXE);
        case game_name::CC3:
            return m_data_directory.load_levels(data_directory::CC3_EXE);
        default:
            RAISE_EX("Not implemented yet");
    }

}

factory::factory(game_name name, const std::filesystem::path& path_to_data)
    : m_game_name(name), m_data_directory(path_to_data) {
}
