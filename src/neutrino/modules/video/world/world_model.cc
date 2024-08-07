//
// Created by igor on 7/5/24.
//

#include "neutrino/modules/video/world/world_model.hh"

namespace neutrino::tiled {
	world_model::world_model()
		: m_world_width(0),
		  m_world_height(0),
		  m_tile_width(0),
		  m_tile_height(0) {
	}

	void world_model::add_animation(const tile& base_tile, const tile& tile_,
	                                const std::chrono::milliseconds& duration) {
		animation_sequence::frame frame(tile_, duration);
		auto itr = m_animations.find(base_tile);
		if (itr == m_animations.end()) {
			animation_sequence seq;
			seq.add_frame(frame);
			m_animations.insert(std::make_pair(base_tile, seq));
		} else {
			itr->second.add_frame(frame);
		}
	}

	void world_model::add_animation(const animation_sequence& seq) {
		m_animations.insert(std::make_pair(seq.get_first_tile(), seq));
	}

	const std::map <tile, animation_sequence>& world_model::get_animations() const {
		return m_animations;
	}

	world_model::container_t::const_iterator world_model::begin() const {
		return m_layers.begin();
	}

	world_model::container_t::const_iterator world_model::end() const {
		return m_layers.end();
	}

	void world_model::set_geometry(unsigned tile_width, unsigned tile_height, unsigned num_tiles_x,
	                               unsigned num_tiles_y) {
		m_tile_width = tile_width;
		m_tile_height = tile_height;
		m_world_width = tile_width * num_tiles_x;
		m_world_height = tile_height * num_tiles_y;
	}

	pixel_coord_t world_model::get_world_width() const {
		return m_world_width;
	}

	pixel_coord_t world_model::get_world_height() const {
		return m_world_height;
	}

	pixel_coord_t world_model::get_tile_width() const {
		return m_tile_width;
	}

	pixel_coord_t world_model::get_tile_height() const {
		return m_tile_height;
	}
}
