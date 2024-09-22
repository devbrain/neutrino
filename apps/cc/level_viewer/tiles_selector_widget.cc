//
// Created by igor on 9/7/24.
//

#include "tiles_selector_widget.hh"



void tiles_selector_widget::show(editor_context& ctx) {
    ImGui::Begin("Tiles Picker");
    const auto& th = ctx.m_tiles_holder;
    auto my_image_texture = th.get_texture().handle();
    constexpr ImVec2 image_size(32.0, 32.0);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f,0.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.f, 1.f, 1.f));
    auto uv_dims = th.get_uv_dims();
    for (int id=0; id<th.size(); id++) {
        ImVec2 uv1 = th.get_uv(id);
        uv1.x += uv_dims.x;
        uv1.y += uv_dims.y;
        ImGui::PushID(id);

        if (id == 10) {
           // ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.f, 0.5f, 0.f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_Button, neutrino::sdl::colors::wheat);
        }

        ImGui::ImageButton("", ImTextureID(my_image_texture), image_size, th.get_uv(id), uv1);
        ImGui::SetItemTooltip("%d", id);

        if (id == 10) {
            ImGui::PopStyleColor();
        }
        if (id % 10 < 9) {
            ImGui::SameLine();
        }
        ImGui::PopID();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::End();
}

