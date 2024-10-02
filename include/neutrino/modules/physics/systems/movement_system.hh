//
// Created by igor on 9/30/24.
//

#ifndef NEUTRINO_INCLUDE_NEUTRINO_MODULES_PHYSICS_SYSTEMS_MOVEMENT_SYSTEM_HH_
#define NEUTRINO_INCLUDE_NEUTRINO_MODULES_PHYSICS_SYSTEMS_MOVEMENT_SYSTEM_HH_

#include <neutrino/neutrino_export.hh>
#include <neutrino/ecs/abstract_system.hh>
#include <neutrino/modules/physics/components/body.hh>
#include <neutrino/modules/physics/detail/quad_tree/quad_tree.hh>
#include <sdlpp/video/geometry.hh>

namespace neutrino::ecs::physics {
	class NEUTRINO_EXPORT movement_system : public abstract_system {
	 public:
		explicit movement_system(const sdl::area_type& world_dimensions);
	 private:
		void update(registry& registry, std::chrono::milliseconds delta_t) override;
		void present(registry& registry) override;
	 private:
		void clamp(body& b) const;
	 private:
		sdl::area_type m_world_dimensions;
	};
}

#endif
