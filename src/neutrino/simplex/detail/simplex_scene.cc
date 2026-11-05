//
// Created by igor on 04/11/2026.
//

#include "simplex/detail/simplex_scene.hh"
#include "simplex/detail/application_adapter.hh"

namespace neutrino::simplex::detail {
    simplex_scene::simplex_scene(application_adapter& adapter)
        : m_adapter(adapter) {
    }

    void simplex_scene::fixed_update(sim_duration dt, const input_snapshot& in) {
        m_adapter.run_update(dt.count(), in);
    }

    void simplex_scene::render() {
        m_adapter.present();
    }
}
