//
// Created by igor on 8/27/24.
//
#include <neutrino/scene/ui_scene.hh>
#include "imgui/imgui.hh"

namespace neutrino {
    ui_scene::ui_scene() {
        imgui::request_for_imgui();
    }

    void ui_scene::setup_imgui() {
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();
    }

    scene::flags ui_scene::get_flags() const {
        return scene::flags::TRANSPARENT | scene::flags::PROPAGATE_EVENTS | scene::flags::PROPAGATE_UPDATES;
    }

    void ui_scene::initialize() {
        scene::initialize();
        imgui::init_imgui_ui([this]() {this->setup_imgui();});
    }

    void ui_scene::handle_input([[maybe_unused]] const sdl::events::event_t& ev) {
    }

    void ui_scene::update([[maybe_unused]] std::chrono::milliseconds delta_time) {
    }

    void ui_scene::render(sdl::renderer& renderer) {
        if (imgui::start_frame()) {
            draw_imgui(renderer);
            imgui::render_imgui(renderer);
        }
    }
}
