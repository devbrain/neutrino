//
// Created by igor on 9/7/24.
//

#ifndef  EDITOR_CONTEXT_HH
#define  EDITOR_CONTEXT_HH

#include <neutrino/modules/properties/properties.hh>

enum class command_mode_t {
    NONE,
    SPRITES
};

struct editor_context {

    editor_context();

    command_mode_t command_mode;
    neutrino::properties properties;
};



#endif
