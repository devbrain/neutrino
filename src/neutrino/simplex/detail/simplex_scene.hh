//
// Created by igor on 04/11/2026.
//

#pragma once

#include <neutrino/scene/base_scene.hh>

namespace neutrino::simplex::detail {
    class application_adapter;

    // The one scene of a simplex application: every fixed tick runs the user's on_update into
    // the canvas, every render draws the canvas. The adapter owns the canvas and the lifecycle;
    // on_create / on_destroy stay in its ready() / teardown(), so a failed on_create still
    // fails startup. Events and resizes keep the base_scene defaults (ignored): input comes
    // from the snapshot and the canvas has a fixed size.
    class simplex_scene final : public base_scene {
        public:
            explicit simplex_scene(application_adapter& adapter);

        private:
            void fixed_update(sim_duration dt, const input_snapshot& in) override;
            void render() override;

            application_adapter& m_adapter;
    };
}
