//
// Created by igor on 7/4/24.
//

#include <neutrino/modules/video/world/color_layer.hh>

namespace neutrino::tiled {
	color_layer::color_layer(const sdl::color& c)
		: m_color(c) {
	}

	sdl::color color_layer::get_color() const {
		return m_color;
	}
}
