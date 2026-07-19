//
// Created by igor on 12/07/2026.
//

#pragma once

#include <memory>

#include <neutrino/scene/base_scene.hh>
#include <neutrino/video/world/world_compositor.hh>
#include <neutrino/video/world/world_renderer.hh>
#include <neutrino/world/world.hh>

#include <ke/game/actors_layer.hh>

// Experiment: show the first KE level via the ke_assets framework, with a mouse-movable
// paddle drawn through the sprite-def pipeline.
class play_game_scene : public neutrino::base_scene {
    public:
        void on_enter() override;
        void on_exit() override;
        void update_physics(neutrino::frame_duration delta_t) override;
        void render(neutrino::frame_duration time_since_last_frame) override;
        void handle_action(const sdlpp::event& ev) override;
        [[nodiscard]] bool is_opaque() const override;

    private:
        std::unique_ptr <neutrino::world_renderer> m_renderer;
        actors_layer m_actors;
        std::unique_ptr <neutrino::world_compositor> m_compositor;

        float m_paddle_center_x{160.0f}; // render-space x driven by the mouse
        int m_paddle_w{32};
        int m_paddle_h{8};
        bool m_ready{false};
        bool m_logged_first_frame{false};
};
