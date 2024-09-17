//
// Created by igor on 9/17/24.
//

#include "crystal_caves_hud.hh"
#include "tile_names.hh"

crystal_caves_hud::crystal_caves_hud(const ecs_registry& reg, int y_px)
    : hud(reg, y_px) {
}

void crystal_caves_hud::draw_string_green(neutrino::sdl::renderer& renderer, int tile_x, const char* text) const {
    while (*text) {
        auto glyph = HUD_FONTS.get(*text);
        draw_tile(renderer, *glyph.m_atlas, tile_x, glyph.m_tile);
        tile_x++;
        text++;
    }
}

void crystal_caves_hud::draw(neutrino::sdl::renderer& renderer, const neutrino::texture_atlas& atlas) const {
    int pos = 0;
    draw_tile(renderer, atlas, pos, TILE_HUD_DOLLAR);
    pos += 2;

    std::ostringstream os;
    os << m_reg.get_player().score;
    draw_string_green(renderer, pos, os.str().c_str());

    pos = 12;
    draw_tile(renderer, atlas, pos, TILE_HUD_GUN);
    pos += 2;
    os.str("");
    os << m_reg.get_player().bullets;
    draw_string_green(renderer, pos, os.str().c_str());

    pos = 21;
    for (int i=0; i<m_reg.get_player().lives; i++) {
        draw_tile(renderer, atlas, pos, TILE_HUD_HEART);
        pos++;
    }
}