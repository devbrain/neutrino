//
// Created by igor on 9/17/24.
//

#ifndef  FACTORY_HH
#define  FACTORY_HH

#include <memory>
#include <filesystem>
#include <tuple>
#include <vector>

#include <neutrino/modules/video/texture_atlas.hh>

#include "data_loader/data_directory.hh"

enum class game_name {
    CC1,
    CC2,
    CC3
};

class factory {
    public:
        static std::unique_ptr <factory> create(game_name name, const std::filesystem::path& path_to_data);

        [[nodiscard]] game_name get_game_name() const;

        [[nodiscard]] std::tuple <neutrino::texture_id_t, neutrino::texture_id_t> load_title(
                                                                                    neutrino::texture_atlas& atlas,
                                                                                    const neutrino::sdl::renderer& renderer) const;

        [[nodiscard]] neutrino::texture_id_t load_publisher(neutrino::texture_atlas& atlas,
                                                            const neutrino::sdl::renderer& renderer) const;

        void setup_tiles(neutrino::texture_atlas& atlas,
                         const neutrino::sdl::renderer& renderer) const;

        [[nodiscard]] std::vector <raw_level_map> load_levels() const;

    private:
        factory(game_name name, const std::filesystem::path& path_to_data);

    private:
        game_name m_game_name;
        data_directory m_data_directory;
};

#endif
