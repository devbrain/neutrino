//
// Created by igor on 7/16/24.
//

#ifndef LOAD_PICTURE_HH
#define LOAD_PICTURE_HH

#include <filesystem>
#include <sdlpp/sdlpp.hh>
#include "data_loader/data_directory.hh"

neutrino::sdl::surface load_picture(data_directory& d, data_directory::resource_t rc);

#endif
