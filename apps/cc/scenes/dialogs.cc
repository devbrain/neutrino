//
// Created by igor on 7/21/24.
//

#include "dialogs.hh"
#include "scenes_registry.hh"
#include "neutrino/application.hh"

std::shared_ptr <dialog_box> create_main_dialog_box(neutrino::sdl::renderer& r) {
	static constexpr auto txt = R"(    Please select an option
    -----------------------
N)ew Game         I)nstructions
R)estore Game     S)tory
O)rdering Info.   H)igh Scores
F)oreign Orders   C)all our BBS
V)ideo Support    A)bout Apogee
         Q)uit to DOS

               {

)";
	dialog_box::keys_map_t km = {
		{neutrino::sdl::ESCAPE, &dialog_box::pop_scene},
		{neutrino::sdl::Q, []() {dialog_box::push_scene(scene_name_t::QUIT_TO_DOS_DIALOG);}}
	};
	return std::make_shared <dialog_box>(r, txt, km);
}

std::shared_ptr <dialog_box> create_quit_to_dos_dialog_box(neutrino::sdl::renderer& r) {
	static constexpr auto txt = R"(Quit to DOS (Y-N) {)";
	dialog_box::keys_map_t km = {
		{neutrino::sdl::ESCAPE, &dialog_box::pop_scene},
		{neutrino::sdl::Y, [](){ neutrino::application::instance().quit(); }},
		{neutrino::sdl::N, &dialog_box::pop_scene}
	};
	return std::make_shared <dialog_box>(r, txt, km);
}
