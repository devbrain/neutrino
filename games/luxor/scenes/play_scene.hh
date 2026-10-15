//
// Created by igor on 14/10/2026.
//

#pragma once

#include <neutrino/scene/base_scene.hh>

namespace luxor {
    class play_scene : public neutrino::base_scene {
        public:
            play_scene() = default;

            void on_enter() override;

            void fixed_update(neutrino::sim_duration, const neutrino::input_snapshot& in) override;

            void render() override;

            [[nodiscard]] bool is_opaque() const override;

            void handle_action(const sdlpp::event& ev) override;
    };
}
