//
// Created by igor on 10/4/24.
//

#ifndef NEUTRINO_INCLUDE_NEUTRINO_MODULES_VIDEO_COMPONENTS_WORLD_RENDERER_CAMERA_HH_
#define NEUTRINO_INCLUDE_NEUTRINO_MODULES_VIDEO_COMPONENTS_WORLD_RENDERER_CAMERA_HH_

#include <neutrino/neutrino_export.hh>
#include <sdlpp/video/geometry.hh>

namespace neutrino {
	struct NEUTRINO_EXPORT world_renderer_camera {
		world_renderer_camera() = default;

		world_renderer_camera(unsigned int w, unsigned int h)
			: camera_position(0, 0), view_port(w, h) {}

		world_renderer_camera(const sdl::point& cameraPosition, const sdl::area_type& viewPort)
			: camera_position(cameraPosition), view_port(viewPort) {}

		sdl::point camera_position;
		sdl::area_type view_port;
	};
}

#endif
