//
// Created by igor on 7/15/24.
//

#ifndef NEUTRINO_SYSTEMS_VIDEO_TYPES_HH
#define NEUTRINO_SYSTEMS_VIDEO_TYPES_HH


#include <limits>
#include <cstdint>
#include <strong_type/strong_type.hpp>

namespace neutrino {
	namespace detail {
		struct texture_id_t_;
		struct tile_id_t_;
		using underlying_tile_type = uint16_t;
	}

	using texture_id_t = strong::type<uint16_t, detail::texture_id_t_, strong::ordered, strong::equality, strong::ostreamable>;
	using tile_id_t = strong::type<detail::underlying_tile_type, detail::tile_id_t_, strong::ordered, strong::equality, strong::ostreamable>;
	inline constexpr auto EMPTY_TILE_VALUE = std::numeric_limits<detail::underlying_tile_type>::max();
}

#endif
