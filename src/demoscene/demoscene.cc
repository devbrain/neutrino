//
// Created by igor on 03/07/2021.
//

#include <neutrino/demoscene/demoscene.hh>
#include <neutrino/engine/scene_manager.hh>
#include "neutrino/demoscene/vga.hh"
#include "main_scene.hh"

namespace neutrino::demoscene
{
    scene::scene(int w, int h)
    : neutrino::engine::main_window (w, h),
      m_main_scene(nullptr)
    {
    }

    scene::~scene() {
        delete m_main_scene;
    }

    void scene::after_window_opened()
    {
        m_vga = std::unique_ptr<vga>(new vga(*this));
        m_main_scene = new main_scene(this);
        engine::scene_manager::instance().activate(engine::scene_manager::instance().add(m_main_scene));
    }
}
