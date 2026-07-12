//
// Created by igor on 30/06/2026.
//

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <neutrino/neutrino_export.h>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/video/geometry_types.hh>
#include <sdlpp/app/game_application.hh>
#include <sdlpp/input/gamepad.hh>

namespace neutrino {
    /// @brief How a fixed design-resolution surface is fitted to the drawable.
    /// Only consulted when @ref application_config::logical_size is set.
    enum class scale_mode : std::uint8_t {
        letterbox,      ///< preserve aspect, pad with the clear color (black bars)
        integer_scale,  ///< preserve aspect, integer multiples only (crisp pixel art)
        overscan,       ///< preserve aspect, fill the drawable and crop the overflow
        stretch,        ///< fill the drawable, ignoring aspect
    };

    struct application_config {
        std::string title = "Neutrino Application";
        int width = 1280;
        int height = 720;
        sdlpp::window_flags flags = sdlpp::window_flags::resizable;
        int target_fps = 60;
        /// Present in lock-step with the display: 1 = on (default, no tearing),
        /// 0 = off, -1 = adaptive. Applied to the renderer at ready time.
        int vsync = 1;
        /// Open the window in fullscreen. Applied as an extra window flag at
        /// creation, so it composes with @ref flags (no need to set the
        /// fullscreen bit there as well).
        bool fullscreen = false;
        /// Fixed design resolution. Unset (default) selects *reflow*: scenes draw
        /// at the drawable's pixel size and a larger window shows more. Set selects
        /// *logical presentation*: scenes always draw in this coordinate space and
        /// SDL scales it to the drawable using @ref scale. See render_size().
        std::optional <dim> logical_size;
        /// Fit mode for @ref logical_size; ignored in reflow mode.
        scale_mode scale = scale_mode::letterbox;
        /// Render at the drawable's native (HiDPI) pixel resolution instead of in
        /// scaled window points. Input still arrives in window points — map it into
        /// render space with neutrino::to_render_coords().
        bool high_pixel_density = false;
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

            // Fullscreen control (defined protected on game_application; re-exported
            // here as public engine API). Each returns success; a resulting drawable
            // change drives base_scene::on_resize like any other resize.
            using sdlpp::game_application::is_fullscreen;
            using sdlpp::game_application::set_fullscreen;
            using sdlpp::game_application::toggle_fullscreen;

        protected:
            sdlpp::window_config get_window_config() override;

            // Template method overrides for game logic
            virtual void ready() {}
            virtual void update([[maybe_unused]] float dt) {}
            virtual void event([[maybe_unused]] const sdlpp::event& e) {}

            /// @brief Called when a gamepad is connected (also once per pad
            /// already plugged in at startup). gamepad_index is the stable
            /// player slot used by gamepad_button / gamepad_axis; a
            /// reconnected pad reuses the lowest free slot.
            virtual void on_gamepad_connected([[maybe_unused]] int gamepad_index) {}
            /// @brief Called when a gamepad is disconnected. The slot keeps
            /// its index and reads as "nothing pressed" until reused.
            virtual void on_gamepad_disconnected([[maybe_unused]] int gamepad_index) {}

            /// @brief Provide the first scene of the game. Called once after
            /// ready(); the scene is pushed synchronously, so the stack is
            /// populated before the first frame. Return nullptr (the default)
            /// to run without scenes on the plain update() callback, or push
            /// scenes manually from ready() via neutrino::push_scene().
            virtual std::unique_ptr <base_scene> create_initial_scene() { return nullptr; }
        private:
            void on_ready() final;
            void on_update(float dt) final;
            void on_render(sdlpp::renderer& r) final;
            void handle_event(const sdlpp::event& e) final;
            void on_quit() noexcept final;
        private:
            struct impl;
            std::unique_ptr <impl> m_pimpl;
    };
}
