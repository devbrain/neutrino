//
// Created by igor on 7/4/24.
//

#include <neutrino/modules/video/world/image_layer.hh>

namespace neutrino::tiled {
	image_layer::image_layer(texture_id_t tex_id)
		: m_texture_id(tex_id) {
	}

	texture_id_t image_layer::get_texture_id() const {
		return m_texture_id;
	}
}
