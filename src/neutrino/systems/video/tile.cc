//
// Created by igor on 7/18/24.
//
#include <neutrino/systems/video/tile.hh>

namespace neutrino {
	tile::tile()
		: m_texture(INVALID_TEXTURE_VALUE),
		  m_id(EMPTY_TILE_VALUE) {
	}

	tile::tile(tile_id_t tile_id)
		: m_texture(0),
		  m_id(tile_id) {
	}

	tile::tile(texture_id_t tex_id, tile_id_t tile_id)
		: m_texture(tex_id),
		  m_id(tile_id) {
	}

	tile::tile(texture_id_t tex_id, tile_id_t tile_id, bool hor_flip, bool vert_flip, bool diag_flip,
			   int animated)
		: m_texture(tex_id),
		  m_id(tile_id) {
		info.flags.animated = animated ? 1 : 0;
		info.flags.diag_flip = diag_flip ? 1 : 0;
		info.flags.hor_flip = hor_flip ? 1 : 0;
		info.flags.vert_flip = vert_flip ? 1 : 0;
	}
}