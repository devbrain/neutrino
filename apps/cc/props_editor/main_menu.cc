//
// Created by igor on 9/7/24.
//

#include <neutrino/imgui/imgui.h>
#include "main_menu.hh"

void show_main_menu(editor_context& ctx) {
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Commands"))
        {
            if (ImGui::MenuItem("Edit Sprites", "", false, ctx.command_mode != command_mode_t::SPRITES)) {
                ctx.command_mode = command_mode_t::SPRITES;
            }

            if (ImGui::MenuItem("Edit Zopa", "")) {

            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
            if (ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
