//
// Created by igor on 9/8/24.
//

#include "sprites_editor_widget.hh"
#include <neutrino/imgui/imgui.h>
#include "magic_enum/magic_enum.hpp"
#include "sprite_names.hh"

sprites_editor_widget::sprites_editor_widget()
    : is_open(false) {

}

void sprites_editor_widget::show(editor_context& ctx) {

    if (ctx.command_mode != command_mode_t::SPRITES) {
        is_open = false;
        return;
    }

    is_open = true;
    ImGui::Begin("Sprites Editor", &is_open);

    constexpr std::size_t sprites_count = magic_enum::enum_count<sprite_names>();
    if (ImGui::TreeNode("Sprites")) {
        for (std::size_t i = 0; i < sprites_count; ++i) {
            auto sprite_id = magic_enum::enum_cast<sprite_names>(i);
            if (sprite_id) {
                auto top_name = magic_enum::enum_name(*sprite_id);
                ImGui::Text("%s", top_name.data());
                ImGui::TreePop();
            }
        }
    }


    ImGui::End();

    if (!is_open) {
        ctx.command_mode = command_mode_t::NONE;
    }
}


