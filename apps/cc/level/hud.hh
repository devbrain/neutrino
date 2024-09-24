//
// Created by igor on 8/25/24.
//

#ifndef  HUD_HH
#define  HUD_HH

#include <neutrino/modules/video/texture_atlas.hh>
#include <sdlpp/video/render.hh>

class hud {
    public:
        explicit hud(int y_px);
        virtual ~hud();
        virtual void draw(neutrino::sdl::renderer& renderer, const neutrino::texture_atlas& atlas) const = 0;
    protected:
        void draw_tile(neutrino::sdl::renderer& renderer, const neutrino::texture_atlas& atlas, int tile_x, neutrino::tile tile_id) const;
        int m_y;
};

#endif
