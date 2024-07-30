//
// Created by igor on 7/4/24.
//

#ifndef  NEUTRINO_SYSTEMS_VIDEO_WORLD_OBJECTS_LAYER_HH
#define  NEUTRINO_SYSTEMS_VIDEO_WORLD_OBJECTS_LAYER_HH

#include <chrono>
#include <sdlpp/video/geometry.hh>
#include <sdlpp/video/render.hh>

#include <neutrino/ecs/registry.hh>
#include <neutrino/systems/video/texture_atlas.hh>
#include <neutrino/neutrino_export.hh>

namespace neutrino::tiled {
	class NEUTRINO_EXPORT objects_layer {
		public:
			explicit objects_layer(ecs::registry& ecs_registry);
			virtual ~objects_layer();

			virtual void update(std::chrono::milliseconds delta_t, const sdl::rect& viewport) = 0;
			virtual void present(sdl::renderer& r, const sdl::rect& viewport, const texture_atlas& atlas) = 0;
		protected:
			ecs::registry& get_registry();
			const ecs::registry& get_registry() const;
		private:
			ecs::registry& m_registry;
	};
}

#endif
