//
// Created by igor on 9/1/24.
//

#ifndef NEUTRINO_PROPERTIES_PROPERTIES_DB_HH
#define NEUTRINO_PROPERTIES_PROPERTIES_DB_HH

#include <filesystem>
#include <vector>
#include <neutrino/modules/properties/properties.hh>

namespace neutrino {
    NEUTRINO_EXPORT void save_properties(const properties& props, const std::filesystem::path& path);
    NEUTRINO_EXPORT properties load_properties(const std::filesystem::path& path);

    namespace builder {
        NEUTRINO_EXPORT void start_empty_sprite(properties& props, int sprite_id);
        NEUTRINO_EXPORT std::vector<int> list_sprites(const properties& props);
        NEUTRINO_EXPORT void remove_sprite(properties& props, int sprite_id);
        NEUTRINO_EXPORT void start_new_sprite_sequence(properties& props, int sprite_id, int sequence_name);
        NEUTRINO_EXPORT std::vector<int> list_sprite_sequences(const properties& props, int sprite_id);
        NEUTRINO_EXPORT void remove_sprite_sequence(properties& props, int sprite_id, int sequence_name);
        NEUTRINO_EXPORT void add_tile_to_sprite_sequence(properties& props, int sprite_id, int sequence_name, const tile_property& tp);
        NEUTRINO_EXPORT void rename_sprite(properties& props, int old_sprite_id, int new_sprite_id);
    }

}

#endif
