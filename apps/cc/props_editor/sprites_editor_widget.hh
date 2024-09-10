//
// Created by igor on 9/8/24.
//

#ifndef  SPRITES_EDITOR_WIDGET_HH
#define  SPRITES_EDITOR_WIDGET_HH

#include "props_editor/editor_context.hh"

class sprites_editor_widget {
    public:
        sprites_editor_widget();
        void show(editor_context &ctx);
    private:
        bool is_open;
};



#endif
