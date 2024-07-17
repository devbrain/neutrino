//
// Created by igor on 7/15/24.
//

#ifndef NEUTRINO_SYSTEMS_VIDEO_TYPES_HH
#define NEUTRINO_SYSTEMS_VIDEO_TYPES_HH

#include <cstddef>
#include <strong_type/strong_type.hpp>

namespace neutrino {
	namespace detail {
		struct texture_id_t_;
	}
	using texture_id_t = strong::type<std::size_t, detail::texture_id_t_, strong::ordered, strong::equality, strong::ostreamable>;
}

#endif
