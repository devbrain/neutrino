//
// Created by igor on 26/06/2021.
//

#ifndef NEUTRINO_WINDOW_IMPL_HH
#define NEUTRINO_WINDOW_IMPL_HH

#include <hal/sdl/window.hh>
#include <hal/sdl/render.hh>

#include <iostream>

namespace neutrino::hal::detail {
    struct window
    {
        ~window() {
            // workaround for strange out of order destruction
            sdl_renderer.destroy();
            sdl_window.destroy();
        }

        sdl::window::flags_t kind_flag;
        std::optional<window_flags_t> flags;

        int width;
        int height;

        int pos_x;
        int pos_y;

        int before_fullscreen_x;
        int before_fullscreen_y;

        bool windowed;
        bool visible;

        bool mouse_focus;
        bool keyboard_focus;


        sdl::window sdl_window;
        sdl::renderer sdl_renderer;

        template<typename Flag>
        sdl::window::flags_t get_flag(Flag f, sdl::window::flags_t x)
        {
            if (!flags)
            {
                return sdl::window::flags_t::NONE;
            }
            if (*flags & f)
            {
                return x;
            }
            return sdl::window::flags_t::NONE;
        }
    };


}

#endif
