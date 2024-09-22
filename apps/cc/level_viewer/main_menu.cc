//
// Created by igor on 9/7/24.
//

#include <sstream>
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
        if (ImGui::BeginMenu("Level")) {
            for (const auto& lvl : ctx.m_level_maps) {
                ImGui::PushID(lvl.level_num);
                std::ostringstream os;
                os << "Level #" << lvl.level_num;
                if (ImGui::MenuItem(os.str().c_str(), nullptr, lvl.level_num == ctx.m_current_level)) {
                    ctx.m_current_level = lvl.level_num;
                }
                ImGui::PopID();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
