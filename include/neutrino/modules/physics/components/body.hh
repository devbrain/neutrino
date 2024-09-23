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
        sdl::point position{0,0};
		sdl::area_type dimensions {0, 0};
		uint32_t flags {0};
    };
}

#endif
