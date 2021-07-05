//
// Created by igor on 03/07/2021.
//

#include <neutrino/demoscene/demoscene.hh>
#include <neutrino/engine/scene_manager.hh>
#include "video.hh"
#include "main_scene.hh"

namespace neutrino::demoscene
{
    scene::scene(int w, int h)
            : neutrino::engine::application(std::move(std::make_unique<video>(w, h)), nullptr),
              m_main_scene(nullptr)
    {
    }

    scene::~scene()
    {
        delete m_main_scene;
    }

    void scene::setup()
    {
        m_main_scene = new main_scene(this);
        engine::scene_manager::instance().add(m_main_scene);
    }
}
