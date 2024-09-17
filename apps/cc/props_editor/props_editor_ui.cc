//
// Created by igor on 8/28/24.
//

#include "props_editor_ui.hh"
#include "main_menu.hh"

props_editor_ui::props_editor_ui(neutrino::sdl::texture&& img, std::vector <neutrino::sdl::rect>&& rects, editor_context& ctx)
    : m_ctx(ctx),
      m_tiles_selector(std::move(img), std::move(rects)) {
}

void props_editor_ui::draw_imgui(neutrino::sdl::renderer& renderer) {
    show_main_menu(m_ctx);
    m_tiles_selector.show(m_ctx);
}
