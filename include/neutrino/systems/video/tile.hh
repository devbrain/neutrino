//
// Created by igor on 7/18/24.
//

#ifndef NEUTRINO_SYSTEMS_VIDEO_TILE_HH
#define NEUTRINO_SYSTEMS_VIDEO_TILE_HH

#include <neutrino/systems/video/types.hh>
#include <neutrino/neutrino_export.hh>



namespace neutrino {
	namespace tiled {
		class NEUTRINO_EXPORT animation_sequence;
	}
	struct NEUTRINO_EXPORT tile {
		tile();
		explicit tile(tile_id_t tile_id);
		tile(texture_id_t tex_id, detail::underlying_tile_type tile_id);
		tile(texture_id_t tex_id, tile_id_t tile_id);
		tile(texture_id_t tex_id, tile_id_t tile_id, bool hor_flip,
			 bool vert_flip, bool diag_flip, int animated);

		explicit tile(const tiled::animation_sequence& seq);
		tile& operator = (const tiled::animation_sequence& seq);

		tile(const tile& other) = default;
		tile& operator = (const tile& other) = default;

		texture_id_t m_texture;
		tile_id_t m_id;

		union {
			struct {
				int hor_flip: 1;
				int vert_flip: 1;
				int diag_flip: 1;
				int animated: 1;
			} flags;

			uint8_t bit_flags;
		} info{0};
	};

	static_assert(sizeof(tile) == 8);

	inline
	bool operator == (const tile& a, const tile& b) {
		return (a.m_texture == b.m_texture) && (a.m_id == b.m_id);
	}
	inline
	bool operator < (const tile& a, const tile& b) {
		return std::make_tuple(a.m_texture, a.m_id) < std::make_tuple(b.m_texture, b.m_id);
	}



}

#endif
