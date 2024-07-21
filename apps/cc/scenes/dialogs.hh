//
// Created by igor on 7/21/24.
//

#ifndef  DIALOGS_HH
#define  DIALOGS_HH

#include <memory>
#include "scenes/dialog_box.hh"

std::shared_ptr<dialog_box> create_main_dialog_box(neutrino::sdl::renderer& r);
std::shared_ptr<dialog_box> create_quit_to_dos_dialog_box(neutrino::sdl::renderer& r);




#endif
