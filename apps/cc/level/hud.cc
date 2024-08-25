//
// Created by igor on 8/25/24.
//

#include <string>
#include <sstream>

#include "level/hud.hh"
#include "tile_names.hh"



hud::hud(const ecs_registry& reg, int y_px)
    : m_reg(reg), m_y(y_px) {
}

void hud::draw(neutrino::sdl::renderer& renderer, const neutrino::texture_atlas& atlas) const {
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

void hud::draw_string_green(neutrino::sdl::renderer& renderer, int tile_x, const char* text) const {
    while (*text) {
        auto glyph = HUD_FONTS.get(*text);
        draw_tile(renderer, *glyph.m_atlas, tile_x, glyph.m_tile);
        tile_x++;
        text++;
    }
}

void hud::draw_tile(neutrino::sdl::renderer& renderer, const neutrino::texture_atlas& atlas, int tile_x,
    neutrino::tile tile_id) const {
    auto [text_ptr, rect] = atlas.get(tile_id);
    neutrino::sdl::rect dst_rect {HUD_TILE_W*tile_x, m_y, rect.w, rect.h};
    renderer.copy(*text_ptr, rect, dst_rect);
}
