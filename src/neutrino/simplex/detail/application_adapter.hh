//
// Created by igor on 18/10/2026.
//

#pragma once

#include <exception>
#include <string>
#include <neutrino/application.hh>
#include <optional>
#include <neutrino/video/draw.hh>

namespace neutrino::simplex {
    class NEUTRINO_EXPORT application;

    namespace detail {
        // Runs a simplex::application on the engine. The canvas and the user's lifecycle live
        // here; the frame itself runs through the scene stack: create_initial_scene() installs a
        // simplex_scene that calls run_update() on every fixed tick and present() on every render.
        class NEUTRINO_EXPORT application_adapter : public neutrino::application {
            public:
                application_adapter(simplex::application* owner, int width_px, int height_px,
                                    const std::string& title, float scale = 1.0f);

                // One fixed tick of the user's on_update, drawing into the canvas. Never throws:
                // a failure stops the application and is kept for rethrow_failure().
                void run_update(float dt, const input_snapshot& input) noexcept;
                // Draw the canvas to the current target. Never throws: a failure stops the
                // application and is kept for rethrow_failure().
                void present() noexcept;
                // Throw the first failure recorded since the last call, if any. The engine
                // contains scene exceptions (log and pop); SIMPLEX_MAIN calls this after every
                // iteration to report them as SDL_APP_FAILURE instead.
                void rethrow_failure();

            protected:
                void ready() override;
                void teardown() override;
                std::unique_ptr <base_scene> create_initial_scene() override;

            private:
                enum class lifecycle { idle, creating, running, stopped, destroyed };

                void stop() noexcept;
                void fail(std::exception_ptr error) noexcept;

                simplex::application* m_owner;
                dim m_canvas_size;
                std::optional<neutrino::texture> m_canvas;
                sdlpp::fcolor m_canvas_color{0.0f, 0.0f, 0.0f, 1.0f};
                lifecycle m_state = lifecycle::idle;
                std::exception_ptr m_failure;
        };
    }
}
