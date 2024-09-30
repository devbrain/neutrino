//
// Created by igor on 8/7/24.
//

#ifndef NEUTRINO_MODULES_PHYSICS_COMPONENTS_BODY_COMPONENT_HH
#define NEUTRINO_MODULES_PHYSICS_COMPONENTS_BODY_COMPONENT_HH

#include <cstdint>
#include <neutrino/neutrino_export.hh>
#include <sdlpp/video/geometry.hh>

namespace neutrino {
    struct NEUTRINO_EXPORT body {
        sdl::point2f position{0.0f,0.0f};
		sdl::point2f speed {0.0f,0.0f};
		sdl::point2f acceleartion{0.0f, 0.0f};

		sdl::area_type dimensions {0, 0};

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
