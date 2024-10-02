//
// Created by igor on 10/1/24.
//

#ifndef NEUTRINO_INCLUDE_NEUTRINO_MODULES_PHYSICS_COMPONENTS_TYPES_HH_
#define NEUTRINO_INCLUDE_NEUTRINO_MODULES_PHYSICS_COMPONENTS_TYPES_HH_

#include <sdlpp/video/geometry.hh>

namespace neutrino::ecs::physics {
	using vector2f = math::vector<float, 2>;
	using dimensions = sdl::area_typef;
	using rectangle = sdl::rectf;
}

#endif
