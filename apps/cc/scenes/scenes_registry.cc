//
// Created by igor on 7/17/24.
//

#include "scenes_registry.hh"
#include <bsw/exception.hh>

namespace detail {
	void scenes_registry::add(scene_name_t name, const std::shared_ptr<neutrino::scene>& scene) {
		m_scenes.insert(std::make_pair(name, scene));
	}

	std::shared_ptr<neutrino::scene> scenes_registry::get(scene_name_t name) {
		auto i = m_scenes.find(name);
		ENFORCE(i != m_scenes.end());
		return i->second;
	}

	void scenes_registry::remove(scene_name_t name) {
		auto i = m_scenes.find(name);
		ENFORCE(i != m_scenes.end());
		m_scenes.erase(i);
	}
}
