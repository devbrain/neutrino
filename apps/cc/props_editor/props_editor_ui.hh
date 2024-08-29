//
// Created by igor on 8/28/24.
//

#ifndef  PROPS_EDITOR_UI_HH
#define  PROPS_EDITOR_UI_HH

#include <vector>
#include <neutrino/scene/ui_scene.hh>
#include <sdlpp/sdlpp.hh>

class props_editor_ui : public neutrino::ui_scene {
    public:
        props_editor_ui(neutrino::sdl::texture&& img, std::vector<neutrino::sdl::rect>&& rects);
    private:
        void draw_imgui(neutrino::sdl::renderer& renderer) override;
    private:
        void draw_tiles_picker();
    private:
        bool show_another_window;
        neutrino::sdl::texture m_tiles_texture;
        neutrino::sdl::area_type m_img_area;
        std::vector<neutrino::sdl::rect> m_rects;
        std::vector<ImVec2> m_tiles_uv;
        ImVec2 m_tile_size_uv;
};

#endif
