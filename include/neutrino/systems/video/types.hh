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
		using underlying_texture_type = uint16_t;

		struct tile_id_t_;
		using underlying_tile_type = uint16_t;

		struct font_id_t_;
		using underlying_font_type = uint16_t;
	}

	using texture_id_t = strong::type<detail::underlying_texture_type, detail::texture_id_t_, strong::ordered, strong::equality, strong::ostreamable>;
	using tile_id_t = strong::type<detail::underlying_tile_type, detail::tile_id_t_, strong::ordered, strong::equality, strong::ostreamable>;
	using font_id_t = strong::type<detail::underlying_font_type, detail::font_id_t_, strong::ordered, strong::equality, strong::ostreamable>;

	inline constexpr auto INVALID_TEXTURE_VALUE = texture_id_t(std::numeric_limits<detail::underlying_texture_type>::max());
	inline constexpr auto EMPTY_TILE_VALUE = tile_id_t(std::numeric_limits<detail::underlying_tile_type>::max());

	using tile_coord_t = unsigned;
	using pixel_coord_t = unsigned;
}

#endif
