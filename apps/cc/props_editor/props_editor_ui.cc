//
// Created by igor on 8/28/24.
//

#include "props_editor_ui.hh"

props_editor_ui::props_editor_ui()
    : show_another_window(true) {
}

void props_editor_ui::draw_imgui(neutrino::sdl::renderer& renderer) {
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
}
