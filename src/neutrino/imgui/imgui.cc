//
// Created by igor on 8/28/24.
//

#include "imgui.hh"
#include "thirdparty/imgui/imgui_impl_sdl2.h"
#include "thirdparty/imgui/imgui_impl_sdlrenderer2.h"

namespace neutrino::imgui {
    namespace {
        bool imgui_initialized = false;
        bool imgui_requested = false;
        std::function<void()> imgui_setup_fn;
    }

    void request_for_imgui() {
        imgui_requested = true;
    }

    void init_imgui(sdl::window& window, sdl::renderer& renderer) {
        if (imgui_requested && !imgui_initialized) {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGui_ImplSDL2_InitForSDLRenderer(window.handle(), renderer.handle());
            ImGui_ImplSDLRenderer2_Init(renderer.handle());
            if (imgui_setup_fn) {
                imgui_setup_fn();
            } else {
                ImGuiIO& io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
                ImGui::StyleColorsDark();
            }
            imgui_initialized = true;
        }
    }

    void shutdown_imgui() {
        if (imgui_initialized) {
            ImGui_ImplSDLRenderer2_Shutdown();
            ImGui_ImplSDL2_Shutdown();
            ImGui::DestroyContext();
            imgui_initialized = true;
        }
    }

    void handle_imgui_events(const SDL_Event& e) {
        if (imgui_initialized) {
            ImGui_ImplSDL2_ProcessEvent(&e);
        }
    }

    void render_imgui(sdl::renderer& renderer) {
        if (imgui_initialized) {
            ImGui::Render();
            const auto& io = ImGui::GetIO();
            auto [sx, sy] = renderer.get_scaling();
            renderer.set_scaling(io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
            //SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
            //SDL_RenderClear(renderer);
            ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer.handle());
            renderer.set_scaling(sx, sy);
        }
    }

    bool start_frame() {
        if (imgui_initialized) {
            ImGui_ImplSDLRenderer2_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();
            return true;
        }
        return false;
    }

    void init_imgui_ui(const std::function<void()>& init_fn) {
        if (imgui_initialized) {
            init_fn();
        } else {
            imgui_setup_fn = init_fn;
        }
    }
}
