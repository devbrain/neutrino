//
// Created by igor on 04/03/2026.
//

#include <neutrino/scene/base_scene.hh>

namespace neutrino {
    base_scene::~base_scene() = default;

    void base_scene::on_resize([[maybe_unused]] dim size) {
    }

    void base_scene::handle_action([[maybe_unused]] const sdlpp::event& ev) {
    }

    bool base_scene::is_opaque() const {
        return true;
    }
}

