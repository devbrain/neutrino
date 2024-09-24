//
// Created by igor on 8/25/24.
//


#include "level/hud.hh"
#include "tile_names.hh"


hud::hud(int y_px)
	: m_y(y_px) {
}

hud::~hud() = default;

void hud::draw_tile(neutrino::sdl::renderer& renderer, const neutrino::texture_atlas& atlas, int tile_x,
					neutrino::tile tile_id) const {
	auto [text_ptr, rect] = atlas.get(tile_id);
	neutrino::sdl::rect dst_rect{HUD_TILE_W * tile_x, m_y, rect.w, rect.h};
	renderer.copy(*text_ptr, rect, dst_rect);
}
