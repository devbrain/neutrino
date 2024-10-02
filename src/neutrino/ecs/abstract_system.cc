//
// Created by igor on 8/7/24.
//

#include <neutrino/ecs/abstract_system.hh>

namespace neutrino::ecs {
    abstract_system::~abstract_system() = default;

	void abstract_system::present([[maybe_unused]] registry& registry) {
	}

	void abstract_system::start([[maybe_unused]] registry& registry) {
	}
}