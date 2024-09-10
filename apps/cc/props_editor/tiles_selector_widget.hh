//
// Created by igor on 9/7/24.
//

#ifndef  TILES_SELECTOR_WIDGET_HH
#define  TILES_SELECTOR_WIDGET_HH

#include <vector>
#include <sdlpp/sdlpp.hh>
#include <neutrino/imgui/imgui.h>
#include "props_editor/editor_context.hh"

class tiles_selector_widget {
    public:
        tiles_selector_widget(neutrino::sdl::texture&& img, std::vector <neutrino::sdl::rect>&& rects);
        void show(editor_context& ctx);
    private:
        neutrino::sdl::texture m_tiles_texture;
        neutrino::sdl::area_type m_img_area;
        std::vector <neutrino::sdl::rect> m_rects;
        std::vector <ImVec2> m_tiles_uv;
        ImVec2 m_tile_size_uv;
};

#endif
