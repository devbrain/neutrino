//
// Created by igor on 7/17/24.
//

#ifndef  DATA_DIRECTORY_HH
#define  DATA_DIRECTORY_HH

#include <istream>
#include <memory>
#include <filesystem>
#include <map>

#include <sdlpp/sdlpp.hh>
#include <assets/assets.hh>

class data_directory {
	public:
		enum resource_t {
			CC1_APOGEE_SCREEN,
			CC1_UP_MAIN,
			CC1_DOWN_MAIN,
			CC1_EXE,
			CC1_TILES,
			CC1_MINI_TILES1,
			CC1_MINI_TILES2,
			CC1_MINI_TILES3,

			CC2_APOGEE_SCREEN,
			CC2_UP_MAIN,
			CC2_DOWN_MAIN,
			CC2_EXE,
			CC2_TILES,
			CC2_MINI_TILES1,
			CC2_MINI_TILES2,
			CC2_MINI_TILES3,

			CC3_APOGEE_SCREEN,
			CC3_UP_MAIN,
			CC3_DOWN_MAIN,
			CC3_EXE,
			CC3_TILES,
			CC3_MINI_TILES1,
			CC3_MINI_TILES2,
			CC3_MINI_TILES3,
		};
	public:
		explicit data_directory(const std::filesystem::path& root);

		std::unique_ptr<std::istream> get(resource_t rc);
		neutrino::sdl::surface load_picture(resource_t rc);
		neutrino::assets::tileset load_tileset(resource_t rc);
	private:
		std::map<resource_t, std::filesystem::path> m_fs;
};

#endif
