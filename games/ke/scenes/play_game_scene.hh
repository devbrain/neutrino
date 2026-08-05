//
// Created by igor on 12/07/2026.
//

#pragma once

#include <optional>

#include <neutrino/scene/base_scene.hh>
#include <neutrino/video/render_texture.hh>  // the composed backdrop

#include <ke/game/mechanics.hh>
#include <ke/assets/registry.hh>

// The KE gameplay scene: draws the static backdrop texture, then the game's sprites
// (bricks, paddle, balls) through a screen-space sprite_batch. Physics runs in game_mechanics.
//
// The assets are INJECTED, not looked up: they belong to the application, which owns them for
// the whole run and releases them in its teardown(). The scene previously called
// rs::clear_ke_assets() from on_exit -- unpublishing something it did not own -- so a second
// gameplay scene (a level transition, or a restart after game over) would hit the ENFORCE in
// require_ke_assets() the moment it entered. Taking the reference at construction makes the
// dependency visible and gives the scene nothing to clear.
class play_game_scene : public neutrino::base_scene {
    public:
        explicit play_game_scene(rs::ke_assets& assets)
            : m_assets(assets) {
        }

        void on_enter() override;
        void on_exit() override;
        void fixed_update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
        void render() override;
        void handle_action(const sdlpp::event& ev) override;
        [[nodiscard]] bool is_opaque() const override;

    private:
        rs::ke_assets& m_assets;                             // owned by the application
        std::optional <neutrino::render_texture> m_backdrop; // the composed level backdrop
        game_mechanics m_mechanics;

        bool m_ready{false};
};
