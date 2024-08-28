//
// Created by igor on 8/28/24.
//

#ifndef NEUTRINO_IMGUI_IMGUI_HH
#define NEUTRINO_IMGUI_IMGUI_HH

#include <sdlpp/sdlpp.hh>
#include <functional>

namespace neutrino::imgui {
    void request_for_imgui();
    void init_imgui(sdl::window& window, sdl::renderer& renderer);
    void shutdown_imgui();
    void handle_imgui_events(const SDL_Event& e);
    void render_imgui(sdl::renderer& renderer);
    bool start_frame();

    void init_imgui_ui(const std::function<void()>& init_fn);

    struct auto_init {
        auto_init(sdl::window& window, sdl::renderer& renderer) {
            init_imgui(window, renderer);
        }
        ~auto_init() {
            shutdown_imgui();
        }
    };
}

#endif
