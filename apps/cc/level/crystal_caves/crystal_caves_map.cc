//
// Created by igor on 9/17/24.
//
#include <neutrino/modules/video/world/tiles_layer.hh>
#include "data_loader/crystal_caves/cc_map_cell.hh"
#include "crystal_caves_map.hh"
#include "tile_names.hh"
#include "sprite_components_factory.hh"
#include <neutrino/utils/random.hh>

#include <neutrino/modules/physics/components/body.hh>
#include <neutrino/modules/video/systems/sprite_system.hh>

crystal_caves_map::crystal_caves_map(std::vector <raw_level_map> maps)
    : maps_registry(std::move(maps)) {
}

void add_object(const cell& c, ecs_registry& reg, int x, int y) {
	if (!c.is_empty()) {
		neutrino::ecs::entity_builder builder(reg.get_registry());
		neutrino::body phys{};
		phys.dimensions = {TILE_W, TILE_H};
		phys.position = neutrino::sdl::point{x, y};
		phys.flags = c.flags;

		auto eid = builder
			.with_component<neutrino::body>(phys)
			.with_component<neutrino::single_tile_sprite>(create_single_tile_component(c.tile_name_id))
			.build();
	}
}

neutrino::tiled::world_model crystal_caves_map::get_map(int name,
                                                        ecs_registry& reg,
                                                        neutrino::world_renderer& wr) const {
    constexpr int PHYSICAL_FLAG = CC_TILE_FLAG_SOLID |
                                  CC_TILE_FLAG_SOLID_TOP |
                                  CC_TILE_FLAG_DAMAGE |
                                  CC_TILE_FLAG_DEATH;
    auto lvl = get_by_name(name);
    auto tiles_w = neutrino::tile_coord_t{lvl.dims.w};
    auto tiles_h = neutrino::tile_coord_t{lvl.dims.h};
    neutrino::tiled::tiles_layer bg_layer(tiles_w, tiles_h, TILE_W, TILE_H);
    neutrino::tiled::tiles_layer static_layer(tiles_w, tiles_h, TILE_W, TILE_H);
    neutrino::tiled::tiles_layer front_layer(tiles_w, tiles_h, TILE_W, TILE_H);
    neutrino::tiled::world_model wm;

    for (unsigned y = 0; y < lvl.dims.h; y++) {
        for (unsigned x = 0; x < lvl.dims.w; x++) {
            auto index = x + y * lvl.dims.w;
            if (lvl.backround[index] != -1) {
                bg_layer.at(x, y) = create_tile(lvl.backround[index]);
            }
            if (!lvl.cells[index].is_empty()) {
                auto cell = lvl.cells[index];
                if (cell.flags == 0) {
                    static_layer.at(x, y) = create_tile(cell.tile_name_id);
                }
                if (cell.flags & PHYSICAL_FLAG) {
                    add_object(cell, reg, x * TILE_W, y*TILE_H);
                }
                if ((cell.flags & CC_TILE_FLAG_RENDER_IN_FRONT) == CC_TILE_FLAG_RENDER_IN_FRONT) {
                    front_layer.at(x, y) = create_tile(cell.tile_name_id);
                }
                if ((cell.flags & CC_TILE_FLAG_ANIMATED) == CC_TILE_FLAG_ANIMATED) {
                    auto sprites_num = cell.get_property(CC_SPRITES_COUNT, 1);
                    std::vector <int> sprites(sprites_num);
                    for (int i = 0; i < sprites_num; i++) {
                        sprites[i] = cell.tile_name_id + i;
                    }
                    neutrino::random::shuffle(sprites);

                    neutrino::tiled::animation_sequence seq;
                    for (int i = 0; i < sprites_num; i++) {
                        seq.add_frame({create_tile(sprites[i]), std::chrono::milliseconds(200)});
                    }
                    if ((cell.flags & CC_TILE_FLAG_RENDER_IN_FRONT) == CC_TILE_FLAG_RENDER_IN_FRONT) {
                        front_layer.at(x, y) = seq;
                    } else {
                        static_layer.at(x, y) = seq;
                    }
                    wm.add_animation(seq);
                }
            }
        }
    }
    neutrino::tiled::objects_layer objects(reg.get_registry());
    objects.register_system <neutrino::ecs::sprite_system>(wr);
    wm.append(std::move(bg_layer));
    wm.append(std::move(static_layer));
	wm.append(std::move(objects));
    wm.append(std::move(front_layer));
    return wm;
}
