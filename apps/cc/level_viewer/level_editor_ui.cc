//
// Created by igor on 8/28/24.
//

#include "level_editor_ui.hh"
#include "main_menu.hh"

level_editor_ui::level_editor_ui(editor_context& ctx)
    : m_ctx(ctx) {
}

void level_editor_ui::draw_imgui(neutrino::sdl::renderer& renderer) {
    show_main_menu(m_ctx);
    m_tiles_selector.show(m_ctx);
}
