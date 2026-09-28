//
// Neutrino Tutorial 01: A Window and an Empty Scene
//
// In this first step of the platformer tutorial, we set up:
// 1. A subclass of neutrino::application to configure window geometry, logical resolution,
//    and presentation scaling.
// 2. A subclass of neutrino::base_scene to implement our game screen.
// 3. The scene lifecycle, fixed-timestep simulation tick, and rendering.
// 4. SDLPP_MAIN entry point macro.
//

#include <neutrino/application.hh>
#include <neutrino/input/hotkey.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>
#include <neutrino/video/draw.hh>

#include <sdlpp/app/entry_point.hh>
#include <sdlpp/video/color.hh>

#include <iostream>
#include <memory>

namespace {

    // Logical design resolution for our platformer.
    // The game logic and drawing always operate in this coordinate space (640x360),
    // regardless of actual window size or display scaling.
    constexpr int logical_width = 640;
    constexpr int logical_height = 360;

    // Window dimensions at startup.
    constexpr int window_width = 1280;
    constexpr int window_height = 720;

    // =========================================================================
    // The Scene: main_scene
    // =========================================================================
    // A scene represents a single screen or gameplay state in Neutrino.
    // Scenes live on a stack managed by the engine.
    class main_scene final : public neutrino::base_scene {
    public:
        main_scene() = default;

        // --- Lifecycle hooks ---

        // Called once when this scene is pushed onto the stack and becomes active.
        void on_enter() override {
            std::cout << "[Tutorial 01] main_scene::on_enter() - scene activated\n";
        }

        // Called once when this scene is popped from the stack during transition or shutdown.
        void on_exit() override {
            std::cout << "[Tutorial 01] main_scene::on_exit() - scene closing\n";
        }

        // Called when the scene becomes active or when the render space changes.
        // In logical presentation mode, the render space is constant (logical_width x logical_height).
        void on_resize(neutrino::dim size) override {
            std::cout << "[Tutorial 01] main_scene::on_resize() - size: "
                      << size.width << "x" << size.height << "\n";
        }

        // --- Pure virtual members required by base_scene ---

        // Fixed-timestep simulation update.
        // Runs 0..N times per frame with a CONSTANT dt (by default 1/120s = 8.33ms),
        // decoupled from the display refresh rate. This ensures physics and logic
        // behave identically across 60Hz, 144Hz, or variable-rate monitors.
        void fixed_update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override {
            m_sim_ticks++;
            m_sim_time += dt.count();

            // Press Escape to pop the scene and cleanly exit the application.
            if (neutrino::hotkey{sdlpp::scancode::escape}.pressed()) {
                std::cout << "[Tutorial 01] Escape pressed - popping scene to exit\n";
                neutrino::pop_scene();
            }
        }

        // Render the scene.
        // Called once per displayed frame after fixed_update substeps have run.
        // Drawing commands take coordinates in the logical design resolution (640x360).
        void render() override {
            // Draw a calm sky-blue background across the logical screen.
            neutrino::draw_rect_fill(
                neutrino::rect{0, 0, logical_width, logical_height},
                sdlpp::color{90, 160, 230, 255}
            );

            // Draw a placeholder ground strip where our platformer level will be built in later steps.
            constexpr int ground_y = 300;
            constexpr int ground_height = logical_height - ground_y;

            // Green grass layer
            neutrino::draw_rect_fill(
                neutrino::rect{0, ground_y, logical_width, 10},
                sdlpp::color{80, 185, 95, 255}
            );

            // Earth / dirt layer
            neutrino::draw_rect_fill(
                neutrino::rect{0, ground_y + 10, logical_width, ground_height - 10},
                sdlpp::color{110, 75, 45, 255}
            );
        }

        // Handle discrete one-shot SDL events (window focus, text input, custom events).
        // Continuous input (e.g. keyboard movement, mouse coordinates) belongs in fixed_update.
        void handle_action([[maybe_unused]] const sdlpp::event& ev) override {
            // Nothing to handle for an empty scene.
        }

        // Return true if this scene completely covers the display (opaque).
        // The engine uses this to avoid wasting GPU work updating and rendering
        // scenes hidden underneath.
        [[nodiscard]] bool is_opaque() const override {
            return true;
        }

    private:
        uint64_t m_sim_ticks{0};
        float m_sim_time{0.0f};
    };

    // =========================================================================
    // The Application: tutorial_app
    // =========================================================================
    // The application owns the window, renderer, main loop, and scene stack.
    class tutorial_app final : public neutrino::application {
    public:
        tutorial_app()
            : neutrino::application(make_config()) {
        }

    protected:
        // Provide the first scene pushed to the scene stack at startup.
        std::unique_ptr<neutrino::base_scene> create_initial_scene() override {
            return std::make_unique<main_scene>();
        }

    private:
        // Configure initial window size, logical resolution, and presentation mode.
        static neutrino::application_config make_config() {
            neutrino::application_config cfg;
            cfg.title = "Neutrino Tutorial 01: Window and Scene";
            cfg.width = window_width;
            cfg.height = window_height;
            cfg.flags = sdlpp::window_flags::resizable;

            // Set fixed 640x360 design resolution with letterbox scaling.
            // When the user resizes the window, the 16:9 canvas scales cleanly with black bars.
            cfg.logical_size = neutrino::dim{logical_width, logical_height};
            cfg.scale = neutrino::scale_mode::letterbox;

            // Frame pacing defaults:
            // - cfg.target_fps = 60
            // - cfg.vsync = 1
            // - cfg.fixed.period = 1/120s (simulation tick)
            // - cfg.fixed.max_substeps = 5 (stall guard)
            // - cfg.fixed.max_frame = 0.25s (pause/drag clamp)

            return cfg;
        }
    };

} // namespace

// SDLPP_MAIN macro expands to the platform entry point (e.g. main / WinMain / SDL_main)
// and handles initializing the application instance.
SDLPP_MAIN(tutorial_app)
