//
// Created by igor on 8/28/24.
//

#ifndef  LEVEL_EDITOR_UI_HH
#define  LEVEL_EDITOR_UI_HH

#include <vector>
#include <neutrino/scene/ui_scene.hh>
#include <sdlpp/sdlpp.hh>
#include "level_viewer/tiles_selector_widget.hh"
#include "level_viewer/editor_context.hh"

class level_editor_ui : public neutrino::ui_scene {
    public:
        explicit level_editor_ui(editor_context& ctx);

    private:
        void draw_imgui(neutrino::sdl::renderer& renderer) override;
    private:
        editor_context& m_ctx;
        tiles_selector_widget m_tiles_selector;
};

#endif
