//
// Created by igor on 8/7/24.
//

#ifndef NEUTRINO_MODULES_VIDEO_SPRITE_COMPONENT_SPRITE_COMPONENT_HH
#define NEUTRINO_MODULES_VIDEO_SPRITE_COMPONENT_SPRITE_COMPONENT_HH

#include <vector>
#include <neutrino/neutrino_export.hh>
#include <neutrino/modules/video/tile.hh>
#include <neutrino/modules/video/world/animation_sequence.hh>
#include <bsw/mp/all_same.hh>
#include <bsw/exception.hh>

namespace neutrino {
    struct NEUTRINO_EXPORT single_tile_sprite {
		single_tile_sprite(const single_tile_sprite&) = default;
        explicit single_tile_sprite(const tile& sprite);
        tile sprite;
    };

    struct NEUTRINO_EXPORT animated_sprite {
		animated_sprite(const animated_sprite&) = default;
        explicit animated_sprite(const tiled::animation_sequence& sequence);
        std::size_t current_frame;
        std::chrono::milliseconds time_in_state;
        const tiled::animation_sequence& sequence;
    };

    struct NEUTRINO_EXPORT animated_sprite_sequence {
		animated_sprite_sequence(const animated_sprite_sequence&) = default;
        animated_sprite_sequence();
        void add_state(const tiled::animation_sequence& sequence);
        std::size_t current_state;
        std::vector<animated_sprite> states;
    };

	struct NEUTRINO_EXPORT sprite_bank {
		sprite_bank(const sprite_bank&) = default;

		sprite_bank()
		: current(0) {}

		template <typename ... T, class = std::enable_if_t<bsw::mp::all_same_v<tile, T...>>>
		explicit sprite_bank(T&&... args)
		: current(0),
		  sprites{std::forward<T>(args)...} {}

		template <typename ... T, class = std::enable_if_t<bsw::mp::all_same_v<tile_id_t, T...>>>
		sprite_bank(texture_id_t tex_id, tile_id_t arg0, T... args)
			: current(0),
			  sprites{tile(tex_id, arg0), tile(tex_id, args)...} {
		}

		sprite_bank(texture_id_t tex_id, int first, int last)
		: current(0), sprites(last-first+1) {
			for (std::size_t i=0; i<=last-first; i++) {
				sprites[i] = tile{tex_id, tile_id_t {first+i}};
			}
		}

		sprite_bank(texture_id_t tex_id, int sprite_id)
		: current(0), sprites(1) {
			sprites[0] = tile{tex_id, tile_id_t {sprite_id}};
		}

		void next() {
			current++;
			if (current >= sprites.size()) {
				current = 0;
			}
		}
		std::size_t current;
		std::vector<tile> sprites;
	};

	struct NEUTRINO_EXPORT sprite_bank_array {
		sprite_bank_array(const sprite_bank_array& ) = default;

		sprite_bank_array()
		: current(0) {}

		template <typename ... T, class = std::enable_if_t<bsw::mp::all_same_v<sprite_bank, T...> && sizeof...(T) >= 1>>
		explicit sprite_bank_array(T&& ... args)
		: current(0),
		  banks{std::forward<T>(args)...} {}

		void next() {
			ENFORCE(current < banks.size());
			banks[current].next();
		}

		void set_current(std::size_t v) {
			if (v != current) {
				ENFORCE(v < banks.size());
				banks[current].current = 0;
				current = v;
			}
		}
		std::size_t current;
		std::vector<sprite_bank> banks;
	};
}

#endif
