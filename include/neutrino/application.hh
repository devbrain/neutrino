//
// Created by igor on 30/06/2026.
//

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <neutrino/neutrino_export.h>
#include <neutrino/input/input_snapshot.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/globals.hh>
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

    /// @brief Fixed-timestep pacing for the simulation. The sim advances in constant @ref period
    /// ticks, decoupled from the render/display rate: an accumulator runs as many ticks as the frame
    /// delta accrued. @ref max_substeps caps the catch-up after a stall (spiral-of-death guard) and
    /// @ref max_frame clamps a huge frame delta before it feeds the accumulator.
    struct fixed_step_config {
        sim_duration period{1.0f / 120.0f}; ///< Simulation tick length, in seconds.
        int          max_substeps = 5;      ///< Max sim steps per frame.
        sim_duration max_frame{0.25f};      ///< Per-frame dt clamp, in seconds.
    };

    using window_flags = sdlpp::window_flags;
    /// @brief Startup configuration for @ref application -- window geometry,
    /// presentation, and frame pacing. Passed once to the application
    /// constructor; every field has a usable default.
    struct application_config {
        /// Window title bar text.
        std::string title = "Neutrino Application";
        /// Initial window width, in window points.
        int width = 1280;
        /// Initial window height, in window points.
        int height = 720;
        /// SDL window flags applied at creation (resizable by default). Composes
        /// with @ref fullscreen and @ref high_pixel_density.
        window_flags flags = window_flags::resizable;
        /// Target frame rate the main loop paces to, in frames per second.
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
        /// How float draw coordinates reach SDL (see @ref pixel_snap). Integer-scaled pixel art
        /// usually wants pixel_snap::logical; the default passes fractions through untouched.
        pixel_snap snap = pixel_snap::off;
        /// Render at the drawable's native (HiDPI) pixel resolution instead of in
        /// scaled window points. Input still arrives in window points — map it into
        /// render space with neutrino::to_render_coords().
        bool high_pixel_density = false;
        /// Fixed-timestep pacing for the simulation (see @ref fixed_step_config). The default
        /// 120 Hz tick decouples the sim from the display rate and keeps physics deterministic.
        fixed_step_config fixed{};
    };

    /// @brief Base class for a Neutrino game: owns the window, renderer, main
    /// loop, input state, and the scene stack.
    ///
    /// Subclass it and override the protected template-method hooks (@ref ready,
    /// @ref update_scene, @ref render_scene, @ref event, @ref resize_scene, the gamepad callbacks, and
    /// @ref create_initial_scene) to supply game logic; the engine drives them.
    /// Most games run entirely through scenes (see base_scene) and use the default
    /// update/render hooks. Construct with an @ref application_config
    /// to customize the window and presentation.
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

            /// @brief One-time setup hook, called once after the window,
            /// renderer, services, and input subsystems are up but before the
            /// first frame and before create_initial_scene(). Override to load
            /// assets or configure global state. Default: no-op.
            virtual void ready() {}

            /// @brief One-time release hook -- the counterpart to @ref ready.
            ///
            /// Called once during shutdown, **after** the scene stack has been finished (so no
            /// scene can still reach what is released here) and **before** the engine's own
            /// resource cache, renderer, window and audio go away. This is the only point at
            /// which a subclass can destroy GPU- or audio-backed resources it owns with the
            /// services those resources unregister through still alive.
            ///
            /// Without it, anything a subclass holds *by value as a member* is destroyed in its
            /// own destructor -- which runs after the base class has already torn the renderer
            /// down, so a texture handle is released against a dead device. That failure is
            /// silent on a clean exit path and only shows up under valgrind or as a crash on
            /// quit, so releasing here rather than in the destructor is not optional for
            /// anything holding GPU or audio resources.
            ///
            /// Called on every shutdown path that reached @ref ready, including @ref quit -- and
            /// only those. Startup can fail before that point (a window or renderer that could not
            /// be created), and SDL calls the quit path regardless; this hook is skipped then,
            /// because the services it promises were never published and the subclass never built
            /// anything to release. Default: no-op. Must not throw; an escaping exception is
            /// logged and swallowed.
            ///
            /// @note @ref ready having thrown partway does NOT skip it: whatever that call
            ///       allocated before failing still needs releasing, and the services are up.
            virtual void teardown() {}
            /// @brief Per-frame logic hook with elapsed seconds and the frame's input.
            /// Called once after common audio/animation updates and input sampling, even
            /// with no scenes. The snapshot remains valid for the duration of this call.
            /// Default: drive the scene stack's fixed-step scheduler, recover from scene
            /// failures, and quit when a previously populated stack becomes empty.
            /// Override to replace scene updates; call the base implementation explicitly
            /// if the scene stack should also advance. Exceptions from overrides propagate
            /// to the application entry point. Input edges are cleared at frame completion.
            virtual void update_scene(float dt, const input_snapshot& input);
            /// @brief Per-frame drawing hook, called after clearing the renderer.
            /// Default: render the scene stack, including scene-failure handling.
            /// Override to replace scene rendering; call the base implementation explicitly
            /// if the scene stack should also be drawn. The engine owns clearing, presentation,
            /// and end-of-frame input bookkeeping. Exceptions from overrides propagate to the
            /// application entry point.
            virtual void render_scene(sdlpp::renderer& renderer);
            /// @brief Raw SDL event hook, called after input/gamepad bookkeeping and
            /// render-space resize delivery. Default: dispatch scene transitions and
            /// events to the scene stack, containing scene exceptions. Override to
            /// replace scene dispatch; explicitly call application::event(e) to retain it.
            /// Ignored scene-transition requests are reclaimed at shutdown. Exceptions
            /// from overrides propagate to the application entry point.
            virtual void event(const sdlpp::event& e);
            /// @brief Render-space size change, deduplicated by the final event wrapper.
            /// Default: notify the active scene, containing scene exceptions. Override
            /// to replace that delivery; call the base implementation to retain it.
            /// Scene activation delivers its initial size separately. A custom application
            /// can query render_size() in ready() for its initial size. Exceptions from
            /// overrides propagate to the application entry point.
            virtual void resize_scene(dim size);

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
            /// to run without scenes using update_scene() and render_scene(), or push
            /// scenes manually from ready() via neutrino::push_scene().
            virtual std::unique_ptr <base_scene> create_initial_scene() { return nullptr; }
        private:
            void on_ready() final;
            void on_update(float dt) final;
            void on_render(sdlpp::renderer& r) final;
            void handle_event(const sdlpp::event& e) final;
            void on_quit() noexcept final;
            /// @brief Build this frame's input snapshot from the tracked input state (pointer mapped
            /// into render space, mouse-button edge/held), sampled once per frame for update_scene.
            [[nodiscard]] input_snapshot sample_input();
        private:
            struct impl;
            std::unique_ptr <impl> m_pimpl;
    };
}
