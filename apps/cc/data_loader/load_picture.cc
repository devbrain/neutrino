//
// Created by igor on 7/16/24.
//
#include <fstream>
#include "data_manager.hh"
#include "load_picture.hh"

neutrino::sdl::surface load_picture(data_directory& d, data_directory::resource_t rc) {
	return get_data_manager()->load<neutrino::sdl::surface>(*d.get(rc));
}