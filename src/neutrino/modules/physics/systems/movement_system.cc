//
// Created by igor on 9/30/24.
//

#include <neutrino/modules/physics/systems/movement_system.hh>
#include <neutrino/modules/physics/components/body.hh>
#include <neutrino/modules/physics/components/dynamic_object.hh>
#include <neutrino/modules/physics/components/static_object.hh>
#include <bsw/logger/logger.hh>

namespace neutrino::ecs::physics {

	movement_system::movement_system(const sdl::area_type& world_dimensions)
		: m_world_dimensions(world_dimensions) {
	}

	void movement_system::update(registry& registry, std::chrono::milliseconds delta_t) {
		registry.iterate([this, delta_t](entity_id_t eid, body& b, const dynamic_object& dyn_obj) {
			const auto t = delta_t.count();
			  auto v = dyn_obj.speed + dyn_obj.acceleartion*t;
			  b.position += v*t;
			  clamp(b);
		});
	}

	void movement_system::present(registry& registry) {

	}

	void movement_system::clamp(body& b) const {
		if (b.position.x < 0) {
			b.position.x = 0;
		}
		if (b.position.y < 0) {
			b.position.y = 0;
		}
		if (b.position.x + static_cast<float>(b.dimensions.w) >= static_cast<float>(m_world_dimensions.w)) {
			b.position.x = static_cast<float>(m_world_dimensions.w) - b.dimensions.w;
		}
		if (b.position.y + static_cast<float>(b.dimensions.h) >= static_cast<float>(m_world_dimensions.h)) {
			b.position.y = static_cast<float>(m_world_dimensions.h) - b.dimensions.h;
		}
	}

}