//
// Created by igor on 7/15/24.
//
#include <iostream>
#include <neutrino/application.hh>
#include <neutrino/scene/title_scene.hh>
#include "data_loader/data_directory.hh"
#include "scenes/title_scene.hh"
#include "scenes/main_screen_scene.hh"
#include "scenes/scenes_registry.hh"

class cc_application : public neutrino::application {
	public:
		explicit cc_application(const std::filesystem::path& path_to_data);
	private:
		void setup_scenes(neutrino::sdl::renderer& renderer) override;

		std::shared_ptr<neutrino::scene> create_main_scene(neutrino::sdl::renderer& renderer);
		std::shared_ptr<neutrino::scene> create_title_scene(neutrino::sdl::renderer& renderer);
	private:
		data_directory m_data_dir;
};

cc_application::cc_application(const std::filesystem::path& path_to_data)
	: m_data_dir(path_to_data){
}

void cc_application::setup_scenes(neutrino::sdl::renderer& renderer) {
	auto title = create_title_scene(renderer);
	scenes_registry::instance().add(name_t::TITLE_SCREEN, title);
	scenes_registry::instance().add(name_t::MAIN_SCREEN, create_main_scene(renderer));
	get_scene_manager().push(title);
}

std::shared_ptr<neutrino::scene> cc_application::create_main_scene(neutrino::sdl::renderer& renderer) {
	auto title_up_id = get_texture_atlas().add(m_data_dir.load_picture(data_directory::CC1_UP_MAIN), renderer);
	auto title_down_id = get_texture_atlas().add(m_data_dir.load_picture(data_directory::CC1_DOWN_MAIN), renderer);
	return std::make_shared <main_screen_scene>(title_up_id, title_down_id, 15, std::chrono::seconds(3));
}

std::shared_ptr<neutrino::scene> cc_application::create_title_scene(neutrino::sdl::renderer& renderer) {
	auto title_id = get_texture_atlas().add(m_data_dir.load_picture(data_directory::CC1_APOGEE_SCREEN), renderer);
	return std::make_shared <title_scene>(title_id, std::chrono::seconds(5));
}

int main(int argc, char* argv[]) {
	cc_application app("/home/igor/proj/ares/games/CAVES/");
	app.init(320, 200);
	app.run();

	return 0;
}
