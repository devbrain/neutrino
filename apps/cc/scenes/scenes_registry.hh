//
// Created by igor on 7/17/24.
//

#ifndef  SCENES_REGISTRY_HH
#define  SCENES_REGISTRY_HH

#include <memory>
#include <map>
#include <neutrino/scene/scene.hh>
#include <bsw/singleton.hh>

enum class name_t {
	TITLE_SCREEN,
	MAIN_SCREEN
};

namespace detail {
	class scenes_registry {
		public:

		public:
			void add(name_t name, const std::shared_ptr <neutrino::scene>& scene);
			std::shared_ptr <neutrino::scene> get(name_t name);
			void remove(name_t name);
		private:
			std::map <name_t, std::shared_ptr <neutrino::scene>> m_scenes;
	};

}
using scenes_registry = bsw::singleton<detail::scenes_registry>;

#endif
