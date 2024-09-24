//
// Created by igor on 9/23/24.
//

#ifndef  NEUTRINO_APPS_CC_LEVEL_GLOBAL_STATE_HH_
#define  NEUTRINO_APPS_CC_LEVEL_GLOBAL_STATE_HH_

#include <bsw/singleton.hh>

namespace detail {
	struct global_state {

		global_state();

		unsigned int lives;
		unsigned int bullets;
		unsigned int score;

	};
}

using global_state = bsw::singleton<detail::global_state>;

#endif
