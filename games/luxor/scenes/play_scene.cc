//
// Created by igor on 14/10/2026.
//

#include "play_scene.hh"
#include <neutrino/scene/scene_transitions.hh>
#include <neutrino/video/draw.hh>
#include <neutrino/video/sprites.hh>
#include <neutrino/video/globals.hh>
#include <sdlpp/video/color.hh>

namespace luxor {
    void play_scene::on_enter() {
    }

    void play_scene::fixed_update(neutrino::sim_duration, const neutrino::input_snapshot& in) {

        // Press Escape to pop the scene and cleanly exit.
        if (in.pressed(neutrino::hotkey{sdlpp::scancode::escape})) {
            LOG_INFO("Escape pressed - exiting");
            neutrino::pop_scene();
        }
    }

    void play_scene::render() {
        // 1. Draw sky background.
        auto render_size = neutrino::render_size();
        neutrino::draw_rect_fill(
            neutrino::rect{0, 0, render_size.width, render_size.height},
            sdlpp::color{90, 160, 230, 255}
        );
    }
}
