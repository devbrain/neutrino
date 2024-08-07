//
// Created by igor on 7/5/24.
//

#ifndef  NEUTRINO_MODULES_VIDEO_WORLD_ANIMATION_SEQUENCE_HH
#define  NEUTRINO_MODULES_VIDEO_WORLD_ANIMATION_SEQUENCE_HH

#include <vector>
#include <chrono>
#include <neutrino/modules/video/tile.hh>
#include <neutrino/neutrino_export.hh>

namespace neutrino::tiled {
	class NEUTRINO_EXPORT animation_sequence {
		public:
			struct frame {
				frame(const tile& tile_, const std::chrono::milliseconds& duration)
					: m_tile(tile_),
					  m_duration(duration) {
				}

				frame(const frame&) = default;
				frame& operator =(const frame&) = default;

				tile m_tile;
				std::chrono::milliseconds m_duration;
			};

			[[nodiscard]] const std::vector <frame>& get_frames() const;
			void add_frame(const frame& f);
			[[nodiscard]] tile get_first_tile() const;

		private:
			std::vector <frame> m_frames;
	};
}

#endif
