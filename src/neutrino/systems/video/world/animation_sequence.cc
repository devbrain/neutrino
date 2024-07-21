//
// Created by igor on 7/5/24.
//

#include <neutrino/systems/video/world/animation_sequence.hh>

namespace neutrino::tiled {
	const std::vector<animation_sequence::frame>& animation_sequence::get_frames() const {
		return m_frames;
	}

	void animation_sequence::add_frame(const frame& f) {
		m_frames.emplace_back(f);
	}
}
