//
// Created by igor on 8/28/24.
//

#ifndef  CC_APPLICATION_HH
#define  CC_APPLICATION_HH

#include <neutrino/application.hh>
#include <neutrino/ecs/registry.hh>
#include "data_loader/data_directory.hh"
#include "level/maps_registry.hh"

class cc_application : public neutrino::application {
    public:
        explicit cc_application(const std::filesystem::path& path_to_data);
        [[nodiscard]] neutrino::tiled::world_model get_map(int name, neutrino::ecs::registry& reg) const;

    private:
        void load_maps();
        void setup_scenes(neutrino::sdl::renderer& renderer) override;
        void load_tiles(const neutrino::sdl::renderer& renderer);

        std::shared_ptr <neutrino::scene> create_main_scene(neutrino::sdl::renderer& renderer);
        std::shared_ptr <neutrino::scene> create_title_scene(neutrino::sdl::renderer& renderer);
        std::shared_ptr <neutrino::scene> create_main_level(neutrino::sdl::renderer& renderer);

    private:
        data_directory m_data_dir;
        std::unique_ptr <maps_registry> m_maps_registry;
};

#endif
