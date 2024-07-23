//
// Created by igor on 7/23/24.
//

#ifndef NEUTRINO_ECS_TYPES_HH
#define NEUTRINO_ECS_TYPES_HH

#include <limits>
#include <cstdint>
#include <strong_type/strong_type.hpp>

namespace neutrino::ecs {
	namespace detail {
		struct entity_id_t_;
		using underlying_entity_type = uint16_t;
	}

	using entity_id_t = strong::type<detail::underlying_entity_type, detail::entity_id_t_, strong::ordered, strong::equality, strong::ostreamable>;
	inline constexpr auto INVALID_ENTITY_VALUE = entity_id_t(std::numeric_limits<detail::underlying_entity_type>::max());

}

#endif
