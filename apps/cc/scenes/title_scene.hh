//
// Created by igor on 7/16/24.
//

#ifndef  TITLE_SCENE_HH
#define  TITLE_SCENE_HH

#include <neutrino/scene/title_scene.hh>

class title_scene : public neutrino::title_scene {
	public:
		title_scene(neutrino::texture_id_t tid, std::chrono::seconds display_time);
	private:
		void when_done() override;
};

#endif
