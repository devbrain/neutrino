//
// Created by igor on 8/28/24.
//

#ifndef  PROPS_EDITOR_UI_HH
#define  PROPS_EDITOR_UI_HH

#include <neutrino/scene/ui_scene.hh>

class props_editor_ui : public neutrino::ui_scene {
    public:
        props_editor_ui();

    private:
        void draw_imgui(neutrino::sdl::renderer& renderer) override;
    private:
        bool show_another_window;
};

#endif
