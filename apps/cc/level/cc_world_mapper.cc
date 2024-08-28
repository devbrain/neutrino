//
// Created by igor on 8/25/24.
//

#include "cc_world_mapper.hh"
#include <neutrino/modules/video/world/tiles_layer.hh>
#include "tile_names.hh"
#include <neutrino/utils/random.hh>

static neutrino::sdl::area_type get_dimensions(const bg_map_t& m) {
    neutrino::sdl::area_type out{};
    for (const auto& p : m) {
        if (out.w < p.posx) {
            out.w = p.posx;
        }
        if (out.h < p.posy) {
            out.h = p.posy;
        }
    }
    out.h++;
    out.w++;
    return out;
}

neutrino::tiled::world_model cc_create_world_model(const fg_map_t& fg_map, const bg_map_t& bg_map,
                                                   neutrino::ecs::registry& reg) {
    auto dims = get_dimensions(bg_map);

    neutrino::tiled::tiles_layer bg_layer(neutrino::tile_coord_t(dims.w), neutrino::tile_coord_t(dims.h), TILE_W,
                                          TILE_H);
    std::array<neutrino::tile, 7> stars_bg = {
        TILE_STAR_BG_0,
        TILE_STAR_BG_1,
        TILE_STAR_BG_2,
        TILE_STAR_BG_3,
        TILE_STAR_BG_4,
        TILE_STAR_BG_5,
        TILE_STAR_BG_6,
    };
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < dims.w; x++) {
            bg_layer.at(neutrino::tile_coord_t(x), neutrino::tile_coord_t(y)) = stars_bg[neutrino::random::get_int(0, 6)];
        }
    }
    for (int y = 5; y < dims.h-1; y++) {
        for (int x = 1; x < dims.w - 1; x++) {
            switch (x % 4) {
                case 0:
                    bg_layer.at(neutrino::tile_coord_t(x), neutrino::tile_coord_t(y)) = TILE_BG_BRICK_INNER2;
                    break;
                case 1:
                    bg_layer.at(neutrino::tile_coord_t(x), neutrino::tile_coord_t(y)) = TILE_BG_BRICK_LEFT;
                    break;
                case 2:
                    bg_layer.at(neutrino::tile_coord_t(x), neutrino::tile_coord_t(y)) = TILE_BG_BRICK_RIGHT;
                    break;
                default:
                    bg_layer.at(neutrino::tile_coord_t(x), neutrino::tile_coord_t(y)) = TILE_BG_BRICK_INNER1;
                    break;
            }
        }
    }

    neutrino::tiled::tiles_layer static_layer(neutrino::tile_coord_t(dims.w), neutrino::tile_coord_t(dims.h), TILE_W,
                                              TILE_H);
    for (const auto& p : bg_map) {

        if (p.code == 639) {
            static_layer.at(neutrino::tile_coord_t(p.posx), neutrino::tile_coord_t(p.posy)) = TILE_TORCH_ANI;
        } else {
            static_layer.at(neutrino::tile_coord_t(p.posx), neutrino::tile_coord_t(p.posy)) = neutrino::tile(
                MAIN_TEX, p.code);
        }
    }

    neutrino::tiled::tiles_layer fg_layer(neutrino::tile_coord_t(dims.w), neutrino::tile_coord_t(dims.h), TILE_W,
                                          TILE_H);
    for (const auto& p : fg_map) {
        fg_layer.at(neutrino::tile_coord_t(p.posx), neutrino::tile_coord_t(p.posy)) = neutrino::tile(MAIN_TEX, p.code);
    }

    neutrino::tiled::world_model wm;
    wm.append(std::move(bg_layer));
    wm.append(std::move(static_layer));
    wm.append(std::move(fg_layer));
    wm.add_animation(TILE_TORCH_ANI);
    return wm;
}
