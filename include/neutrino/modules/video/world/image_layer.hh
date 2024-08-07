//
// Created by igor on 7/4/24.
//

#ifndef  NEUTRINO_MODULES_VIDEO_WORLD_WORLD_RENDERER_IMAGE_LAYER_HH
#define  NEUTRINO_MODULES_VIDEO_WORLD_WORLD_RENDERER_IMAGE_LAYER_HH

#include <neutrino/modules/video/types.hh>
#include <neutrino/neutrino_export.hh>

namespace neutrino::tiled {
	class NEUTRINO_EXPORT image_layer {
		public:
			explicit image_layer(texture_id_t tex_id);
			[[nodiscard]] texture_id_t get_texture_id() const;
		private:
			texture_id_t m_texture_id;
	};
}

#endif
