//
// Created by igor on 8/7/24.
//

#ifndef NEUTRINO_MODULES_PHYSICS_COMPONENTS_BODY_COMPONENT_HH
#define NEUTRINO_MODULES_PHYSICS_COMPONENTS_BODY_COMPONENT_HH

#include <cstdint>
#include <neutrino/neutrino_export.hh>
#include <neutrino/modules/physics/components/types.hh>

namespace neutrino::ecs::physics {
    struct NEUTRINO_EXPORT body {
        vector2f position{0.0f,0.0f};
		dimensions dimensions {0, 0};

		uint64_t flags {0};

		void set_flag(unsigned f) {
			flags |= (1ull << f);
		}

		void clear_flag(unsigned f) {
			flags &= ~(1ull << f);
		}

		[[nodiscard]] bool has_flag(unsigned f) const {
			return (flags & (1ull << f)) != 0;
		}
    };
}

#endif
