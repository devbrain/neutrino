//
// Created by igor on 8/28/24.
//

#ifndef  PROPS_EDITOR_UI_HH
#define  PROPS_EDITOR_UI_HH

#include <vector>
#include <neutrino/scene/ui_scene.hh>
#include <sdlpp/sdlpp.hh>
#include "props_editor/tiles_selector_widget.hh"
#include "props_editor/editor_context.hh"

class props_editor_ui : public neutrino::ui_scene {
    public:
        props_editor_ui(neutrino::sdl::texture&& img,
                        std::vector <neutrino::sdl::rect>&& rects,
                        editor_context& ctx);

    private:
        void draw_imgui(neutrino::sdl::renderer& renderer) override;

    private:
        editor_context& m_ctx;
        tiles_selector_widget m_tiles_selector;
};

#endif
