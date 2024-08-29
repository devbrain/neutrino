//
// Created by igor on 8/28/24.
//

#include "props_editor_ui.hh"

props_editor_ui::props_editor_ui(neutrino::sdl::texture&& img, std::vector <neutrino::sdl::rect>&& rects)
    : show_another_window(true), m_tiles_texture(std::move(img)), m_rects(std::move(rects)) {
    m_img_area = m_tiles_texture.get_dimensions();

    for (const auto& r : m_rects) {
        m_tiles_uv.emplace_back(static_cast <float>(r.x) / static_cast <float>(m_img_area.w),
                           static_cast <float>(r.y) / static_cast <float>(m_img_area.h));
    }
    m_tile_size_uv = ImVec2(16.0f / static_cast <float>(m_img_area.w), 16.0f / static_cast <float>(m_img_area.h));
}

void props_editor_ui::draw_imgui(neutrino::sdl::renderer& renderer) {
    static int k = 0;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
     //       ShowExampleMenuFile();
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

    if (show_another_window) {
        ImGui::Begin("Another Window", &show_another_window);
        // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
    draw_tiles_picker();

}

void props_editor_ui::draw_tiles_picker() {
    ImGui::Begin("Tiles Picker");
    int id = 0;
    const auto my_image_texture = m_tiles_texture.handle();
    const ImVec2 image_size(32.0, 32.0);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f,0.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.f, 1.f, 1.f));

    for (const auto& uv: m_tiles_uv) {
        ImVec2 uv1 = uv;
        uv1.x += m_tile_size_uv.x;
        uv1.y += m_tile_size_uv.y;
        ImGui::PushID(id);

        if (id == 10) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.f, 0.5f, 0.f, 1.f));
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
