//
// Created by igor on 7/4/24.
//

#ifndef  NEUTRINO_MODULES_VIDEO_WORLD_COLOR_LAYER_HH
#define  NEUTRINO_MODULES_VIDEO_WORLD_COLOR_LAYER_HH

#include <sdlpp/video/color.hh>
#include <neutrino/neutrino_export.hh>

namespace neutrino::tiled {
	class NEUTRINO_EXPORT color_layer {
		public:
			explicit color_layer(const sdl::color& c);
			[[nodiscard]] sdl::color get_color() const;
		private:
			sdl::color m_color;
	};
}

#endif
