//
// Created by igor on 9/17/24.
//

#ifndef  CRYSTAL_CAVES_HUD_HH
#define  CRYSTAL_CAVES_HUD_HH

#include "level/hud.hh"

class crystal_caves_hud : public hud {
    public:
        crystal_caves_hud(int y_px);

        void draw(neutrino::sdl::renderer& renderer, const neutrino::texture_atlas& atlas) const override;
    private:
        void draw_string_green(neutrino::sdl::renderer& renderer, int tile_x, const char* text) const;
};

#endif
