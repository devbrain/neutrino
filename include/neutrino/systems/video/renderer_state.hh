//
// Created by igor on 7/21/24.
//

#ifndef NEUTRINO_SYSTEMS_VIDEO_RENDERER_STATE_HH
#define NEUTRINO_SYSTEMS_VIDEO_RENDERER_STATE_HH

#include <sdlpp/sdlpp.hh>
#include <neutrino/neutrino_export.hh>

namespace neutrino {
	struct NEUTRINO_EXPORT render_state {
		sdl::renderer& m_renderer;
		std::optional <sdl::texture> m_target;

		explicit render_state(sdl::renderer& r)
			: m_renderer(r),
			  m_target(r.get_target()) {
		}

		~render_state() {
			if (m_target) {
				m_renderer.set_target(*m_target);
			} else {
				m_renderer.restore_default_target();
			}
		}
	};
}

#endif
