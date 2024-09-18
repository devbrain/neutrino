//
// Created by igor on 7/21/24.
//

#ifndef  DIALOGS_HH
#define  DIALOGS_HH

#include <memory>
#include "scenes/dialog_box.hh"
#include "scenes_registry.hh"
#include "factory.hh"

class dialogs_factory {
    public:
        dialogs_factory(game_name name, text_resource&& text);

        void create_dialogs(neutrino::sdl::renderer& r) const;
    private:
        void create_main_dialog_box(neutrino::sdl::renderer& r) const;
        void create_quit_to_dos_dialog_box(neutrino::sdl::renderer& r) const;
        void create_instructions(neutrino::sdl::renderer& r) const;

        [[nodiscard]] std::string get_text(scene_name_t scene_name) const;
    private:
        game_name name;
        text_resource text;
};

#endif
