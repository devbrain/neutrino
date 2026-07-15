//
// Created by igor on 12/07/2026.
//

#pragma once

#include <memory>

#include <neutrino/scene/base_scene.hh>
#include <neutrino/video/world/render_layer.hh>
#include <neutrino/video/world/sprite_batch.hh>
#include <neutrino/video/world/world_compositor.hh>
#include <neutrino/video/world/world_renderer.hh>
#include <neutrino/world/world.hh>

#include "resources/ke_assets.hh"
#include "resources/ke_loader.hh"

// The game-drawn "actors" layer: fills the sprite_batch with the moving sprites (just the
// paddle for now). A world_compositor slots it right after the brick tile layer.
class ke_actors_layer : public neutrino::render_layer {
    public:
        neutrino::sprite_visual_ref paddle;
        neutrino::world_point paddle_pos{0.0f, 0.0f}; // sprite pivot position, world/screen pixels

        void draw(const neutrino::layer_view& view, neutrino::sprite_batch& batch) override;
};

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
        rs::game_resources m_res;
        rs::ke_assets m_assets; // owned; published via set_ke_assets
        neutrino::world m_world;
        std::unique_ptr <neutrino::world_renderer> m_renderer;
        ke_actors_layer m_actors;
        std::unique_ptr <neutrino::world_compositor> m_compositor;

        float m_paddle_center_x{160.0f}; // render-space x driven by the mouse
        int m_paddle_w{32};
        int m_paddle_h{8};
        bool m_ready{false};
        bool m_logged_first_frame{false};
};
