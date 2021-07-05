//
// Created by igor on 03/07/2021.
//

#include <neutrino/engine/scene.hh>

namespace neutrino::engine
{

    scene::scene(int scene_id)
    : m_id(scene_id) {}

    scene::~scene() = default;

    int scene::id() const noexcept {
        return m_id;
    }
}