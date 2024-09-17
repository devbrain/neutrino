//
// Created by igor on 8/25/24.
//

#ifndef  MAIN_LEVEL_SCENE_HH
#define  MAIN_LEVEL_SCENE_HH

#include <memory>
#include <neutrino/scene/scene.hh>
#include <neutrino/modules/video/world/world_model.hh>
#include <neutrino/modules/video/world_renderer.hh>
#include "level/ecs_registry.hh"
#include "level/crystal_caves/crystal_caves_hud.hh"
#include "level/maps_registry.hh"

class main_level_scene : public neutrino::scene {
    public:
        main_level_scene(neutrino::sdl::renderer& r,
                         const neutrino::sdl::area_type& window_size,
                         const maps_registry& maps);
    private:
        void update(std::chrono::milliseconds delta_time) override;
        void render(neutrino::sdl::renderer& renderer) override;
        void initialize() override;

        neutrino::world_renderer m_world_renderer;
        ecs_registry m_ecs;
        neutrino::tiled::world_model m_world_model;
        crystal_caves_hud m_hud;
};

#endif
