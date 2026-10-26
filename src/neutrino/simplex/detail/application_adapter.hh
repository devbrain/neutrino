//
// Created by igor on 18/10/2026.
//

#pragma once

#include <string>
#include <neutrino/application.hh>
#include <optional>
#include <neutrino/video/draw.hh>

namespace neutrino::simplex {
    class NEUTRINO_EXPORT application;

    namespace detail {
        class NEUTRINO_EXPORT application_adapter : public neutrino::application {
            public:
                application_adapter(simplex::application* owner, int width_px, int height_px,
                                    const std::string& title, float scale = 1.0f);

            protected:
                void ready() override;
                void update_scene(float dt, const input_snapshot& input) override;
                void render_scene() override;
                void teardown() override;
                // Input is delivered through the frame snapshot. SDL presentation
                // scales the fixed canvas; neither hook dispatches into scenes.
                void event(const neutrino::event&) override {}
                void resize_scene(dim) override {}

            private:
                enum class lifecycle { idle, creating, running, stopped, destroyed };

                void stop() noexcept;

                simplex::application* m_owner;
                dim m_canvas_size;
                std::optional<neutrino::texture> m_canvas;
                sdlpp::fcolor m_canvas_color{0.0f, 0.0f, 0.0f, 1.0f};
                lifecycle m_state = lifecycle::idle;
        };
    }
}
