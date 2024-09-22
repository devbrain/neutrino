//
// Created by igor on 9/7/24.
//

#ifndef  EDITOR_CONTEXT_HH
#define  EDITOR_CONTEXT_HH

#include "data_loader/raw_level_map.hh"
#include "level_viewer/tiles_holder.hh"

enum class command_mode_t {
    NONE,
    SPRITES
};

struct editor_context {

    editor_context();

    command_mode_t command_mode;
    std::vector<raw_level_map> m_level_maps;
    int m_current_level;
    tiles_holder m_tiles_holder;
    tiles_holder m_fonts;
};



#endif
