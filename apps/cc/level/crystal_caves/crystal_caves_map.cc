//
// Created by igor on 9/17/24.
//
#include <neutrino/modules/video/world/tiles_layer.hh>
#include "data_loader/crystal_caves/cc_map_cell.hh"
#include "data_loader/crystal_caves/cc_tile_ids.hh"
#include "crystal_caves_map.hh"
#include "tile_names.hh"
#include "sprite_components_factory.hh"
#include "crystal_caves_sprite_states.hh"
#include <neutrino/utils/random.hh>

#include <neutrino/modules/physics/components/body.hh>
#include <neutrino/modules/video/systems/sprite_system.hh>

crystal_caves_map::crystal_caves_map(std::vector <raw_level_map> maps)
    : maps_registry(std::move(maps)) {
}

void add_object(const cell& c, ecs_registry& reg, int x, int y) {
	neutrino::ecs::entity_builder builder(reg.get_world());
	neutrino::body phys{};
	phys.dimensions = {TILE_W, TILE_H};
	phys.position = neutrino::sdl::point{x, y};

	if (c.has_property(CC_CELL_TYPE)) {
		auto cell_type = c.get_property(CC_CELL_TYPE, 0);
		if (cell_type == CC_CELL_PLAYER_START) {
			auto eid = builder.with_component<neutrino::body>(phys)
			    .with_component<neutrino::sprite_bank_array>(create_mylo_sprite_component()).build();
			reg.set_player(eid);
			return;
		}
	}

	if (!c.is_empty()) {
		phys.flags = c.flags;
		(void)builder.with_component<neutrino::body>(phys)
					 .with_component<neutrino::single_tile_sprite>(create_single_tile_component(c.tile_name_id))
					 .build();
	}
}

std::unique_ptr<level> crystal_caves_map::get_map(int name, neutrino::world_renderer& wr) const {
    constexpr int PHYSICAL_FLAG = CC_TILE_FLAG_SOLID |
                                  CC_TILE_FLAG_SOLID_TOP |
                                  CC_TILE_FLAG_DAMAGE |
                                  CC_TILE_FLAG_DEATH;
	auto out = std::make_unique<level>();

    auto lvl = get_by_name(name);
    auto num_tiles_hor = neutrino::tile_coord_t{lvl.dims.w};
    auto num_tiles_vert = neutrino::tile_coord_t{lvl.dims.h};
    neutrino::tiled::tiles_layer bg_layer(num_tiles_hor, num_tiles_vert, TILE_W, TILE_H);
    neutrino::tiled::tiles_layer static_layer(num_tiles_hor, num_tiles_vert, TILE_W, TILE_H);
    neutrino::tiled::tiles_layer front_layer(num_tiles_hor, num_tiles_vert, TILE_W, TILE_H);
    auto& wm = out->get_model();
	wm.set_geometry(TILE_W, TILE_H, num_tiles_hor, num_tiles_vert);

    for (unsigned y = 0; y < lvl.dims.h; y++) {
        for (unsigned x = 0; x < lvl.dims.w; x++) {
            auto index = x + y * lvl.dims.w;
            if (lvl.backround[index] != -1) {
                bg_layer.at(x, y) = create_tile(lvl.backround[index]);
            }
			bool obj_added = false;
			auto cell = lvl.cells[index];
            if (!cell.is_empty()) {
                if (cell.flags == 0) {
                    static_layer.at(x, y) = create_tile(cell.tile_name_id);
                }
                if (cell.flags & PHYSICAL_FLAG) {
                    add_object(cell, out->get_registry(), x * TILE_W, y*TILE_H);
					obj_added = true;
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
                        front_layer.at(x, y) = create_tile(sprites[0]);
						front_layer.at(x, y).info.flags.animated = true;
                    } else {
                        static_layer.at(x, y) = create_tile(sprites[0]);;
						static_layer.at(x, y).info.flags.animated = true;
                    }
                    wm.add_animation(seq);
					obj_added = true; // do not add animated objects to objects layer
                }
            }
			if (!obj_added && cell.is_interesting()) {
				add_object(cell, out->get_registry(), x * TILE_W, y*TILE_H);
				obj_added = true;
			}
        }
    }
    neutrino::tiled::objects_layer objects(out->get_registry().get_world());
    objects.register_system <neutrino::ecs::sprite_system>(wr);
	auto& player = out->get_registry().get_component<neutrino::body>();
	auto& mylo = out->get_registry().get_component<neutrino::sprite_bank_array>();
	if (player.position.x / TILE_W < num_tiles_hor / 2.0f) {
		mylo.current = SPRITE_STATE_MOVE_RIGHT;
	} else {
		mylo.current = SPRITE_STATE_MOVE_LEFT;
	}
    wm.append(std::move(bg_layer));
    wm.append(std::move(static_layer));
	wm.append(std::move(objects));
    wm.append(std::move(front_layer));
    return out;
}
