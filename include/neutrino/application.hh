//
// Created by igor on 30/06/2026.
//

#pragma once

#include <memory>
#include <string>

#include <neutrino/neutrino_export.h>
#include <sdlpp/app/game_application.hh>
#include <sdlpp/input/gamepad.hh>

namespace neutrino {
    struct application_config {
        std::string title = "Neutrino Application";
        int width = 1280;
        int height = 720;
        sdlpp::window_flags flags = sdlpp::window_flags::resizable;
        int target_fps = 60;
    };

    class NEUTRINO_EXPORT application : public sdlpp::game_application {
        public:
            application();
            explicit application(const application_config& cfg);
            ~application() override;

            using sdlpp::game_application::get_key;
            using sdlpp::game_application::get_mouse;
            using sdlpp::game_application::get_mouse_x;
            using sdlpp::game_application::get_mouse_y;
            using sdlpp::game_application::get_mouse_pos;
            using sdlpp::game_application::get_mouse_wheel;

            [[nodiscard]] sdlpp::button_state get_gamepad_button_state(int gamepad_index, sdlpp::gamepad_button button) const noexcept;
            [[nodiscard]] float get_gamepad_axis(int gamepad_index, sdlpp::gamepad_axis axis) const noexcept;

        protected:
            sdlpp::window_config get_window_config() override;
            void on_ready() final;
            void on_update(float dt) final;
            void on_render(sdlpp::renderer& r) final;
            void handle_event(const sdlpp::event& e) final;

            // Template method overrides for game logic
            virtual void ready() {}
            virtual void update([[maybe_unused]] float dt) {}
            virtual void render([[maybe_unused]] sdlpp::renderer& r) {}
            virtual void event([[maybe_unused]] const sdlpp::event& e) {}

        private:
            struct impl;
            std::unique_ptr <impl> m_pimpl;
    };
}
