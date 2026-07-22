//
// Created by igor on 12/07/2026.
//

#pragma once

#include <optional>

#include <neutrino/scene/base_scene.hh>
#include <neutrino/video/render_texture.hh>  // the composed backdrop

#include <ke/game/mechanics.hh>

// The KE gameplay scene: draws the static backdrop texture, then the game's sprites
// (bricks, paddle, balls) through a screen-space sprite_batch. Physics runs in game_mechanics.
class play_game_scene : public neutrino::base_scene {
    public:
        void on_enter() override;
        void on_exit() override;
        void update_physics(neutrino::frame_duration delta_t) override;
        void render(neutrino::frame_duration time_since_last_frame) override;
        void handle_action(const sdlpp::event& ev) override;
        [[nodiscard]] bool is_opaque() const override;

    private:
        std::optional <neutrino::render_texture> m_backdrop; // the composed level backdrop
        game_mechanics m_mechanics;

        int m_paddle_target_x{160}; // desired paddle centre (render x), driven by the mouse
        bool m_ready{false};
};
