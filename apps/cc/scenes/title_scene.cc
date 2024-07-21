//
// Created by igor on 7/16/24.
//

#include "title_scene.hh"
#include "scenes_registry.hh"

title_scene::title_scene(neutrino::texture_id_t tid, std::chrono::seconds display_time): neutrino::title_scene(tid, display_time) {
}

void title_scene::when_done() {
	replace_scene(scenes_registry::instance().get(scene_name_t::MAIN_SCREEN));
	scenes_registry::instance().remove(scene_name_t::TITLE_SCREEN);
}
