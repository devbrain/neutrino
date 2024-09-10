//
// Created by igor on 9/7/24.
//

#include "tiles_selector_widget.hh"

tiles_selector_widget::tiles_selector_widget(neutrino::sdl::texture&& img, std::vector<neutrino::sdl::rect>&& rects)
: m_tiles_texture(std::move(img)), m_rects(std::move(rects)) {
    m_img_area = m_tiles_texture.get_dimensions();

    for (const auto& r : m_rects) {
        m_tiles_uv.emplace_back(static_cast <float>(r.x) / static_cast <float>(m_img_area.w),
                           static_cast <float>(r.y) / static_cast <float>(m_img_area.h));
    }
    m_tile_size_uv = ImVec2(16.0f / static_cast <float>(m_img_area.w), 16.0f / static_cast <float>(m_img_area.h));
}

void tiles_selector_widget::show(editor_context& ctx) {
    ImGui::Begin("Tiles Picker");
    int id = 0;
    const auto my_image_texture = m_tiles_texture.handle();
    constexpr ImVec2 image_size(32.0, 32.0);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f,0.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.f, 1.f, 1.f));

    for (const auto& uv: m_tiles_uv) {
        ImVec2 uv1 = uv;
        uv1.x += m_tile_size_uv.x;
        uv1.y += m_tile_size_uv.y;
        ImGui::PushID(id);

        if (id == 10) {
           // ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.f, 0.5f, 0.f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_Button, neutrino::sdl::colors::wheat);
        }

        ImGui::ImageButton("", my_image_texture, image_size, uv, uv1);
        ImGui::SetItemTooltip("%d", id);

        if (id == 10) {
            ImGui::PopStyleColor();
        }
        if (id % 10 < 9) {
            ImGui::SameLine();
        }
        id++;

        ImGui::PopID();
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::End();
}

