//
// Created by igor on 9/1/24.
//

#include <fstream>
#include <neutrino/modules/properties/properties_db.hh>

namespace neutrino {
    // void save_properties(const properties& props, const std::filesystem::path& path) {
    //     std::ofstream out(path);
    //     serialize_to_yaml(props, out);
    // }
    //
    // properties load_properties(const std::filesystem::path& path) {
    //     std::ifstream in(path);
    //     return deserialize_from_yaml<properties>(in);
    // }



    namespace builder {
        void start_empty_sprite(properties& props, int sprite_id) {
            props.sprites.emplace(sprite_id, sprite_properties{});
        }

        std::vector<int> list_sprites(const properties& props) {
            std::vector<int> sprites;
            for (const auto& [fst, snd] : props.sprites) {
                sprites.emplace_back(fst);
            }
            return sprites;
        }

        void remove_sprite(properties& props, int sprite_id) {
            auto itr = props.sprites.find(sprite_id);
            if (itr != props.sprites.end()) {
                props.sprites.erase(itr);
            }
        }

        void start_new_sprite_sequence(properties& props, int sprite_id, int sequence_name) {
            auto itr = props.sprites.find(sprite_id);
            if (itr == props.sprites.end()) {
                sprite_properties sp;
                sp.object_id = sprite_id;
                sp.tiles.emplace(sequence_name, tiles_sequence{});
                props.sprites.emplace(sprite_id, sp);
            } else {
                itr->second.object_id = sprite_id;
                itr->second.tiles.emplace(sequence_name, tiles_sequence{});
            }
        }

        std::vector<int> list_sprite_sequences(const properties& props, int sprite_id) {
            std::vector<int> out;
            auto itr = props.sprites.find(sprite_id);
            if (itr != props.sprites.end()) {
                for (const auto& [fst, snd] : itr->second.tiles) {
                    out.emplace_back(fst);
                }
            }
            return out;
        }

        void remove_sprite_sequence(properties& props, int sprite_id, int sequence_name) {
            auto itr = props.sprites.find(sprite_id);
            if (itr != props.sprites.end()) {
                auto titr = itr->second.tiles.find(sequence_name);
                if (titr != itr->second.tiles.end()) {
                    itr->second.tiles.erase(titr);
                }
            }
        }

        void add_tile_to_sprite_sequence(properties& props, int sprite_id, int sequence_name, const tile_property& tp) {
            auto itr = props.sprites.find(sprite_id);
            if (itr == props.sprites.end()) {
                sprite_properties sp;
                sp.object_id = sprite_id;
                sp.tiles.emplace(sequence_name, tp);
                props.sprites.emplace(sprite_id, sp);
            } else {
                itr->second.object_id = sprite_id;
                auto titr = itr->second.tiles.find(sequence_name);
                if (titr != itr->second.tiles.end()) {
                    titr->second.sequence.emplace_back(tp);
                } else {
                    itr->second.tiles.emplace(sequence_name, tp);
                }
            }
        }

        void rename_sprite(properties& props, int old_sprite_id, int new_sprite_id) {
            auto itr = props.sprites.find(old_sprite_id);
            if (itr != props.sprites.end()) {
                itr->second.object_id = new_sprite_id;
                auto temp = itr->second;
                props.sprites.erase(itr);
                props.sprites.emplace(new_sprite_id, temp);
            }
        }
    }
}
