//
// Created by igor on 10/1/24.
//

#ifndef NEUTRINO_INCLUDE_NEUTRINO_MODULES_PHYSICS_COMPONENTS_DYNAMIC_OBJECT_HH_
#define NEUTRINO_INCLUDE_NEUTRINO_MODULES_PHYSICS_COMPONENTS_DYNAMIC_OBJECT_HH_

#include <neutrino/modules/physics/components/types.hh>
#include <neutrino/neutrino_export.hh>

namespace neutrino::ecs::physics {
	struct NEUTRINO_EXPORT dynamic_object {
		vector2f speed {0.0f,0.0f};
		vector2f acceleartion{0.0f, 0.0f};
		rectangle boundary;
	};
}

#endif
