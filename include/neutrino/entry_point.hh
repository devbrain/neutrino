//
// Created by igor on 24/10/2026.
//

#pragma once

/**
 * @file entry_point.hh
 * @brief NEUTRINO_MAIN(AppClass): the program entry point for a neutrino::application subclass.
 *
 * Include in exactly one translation unit: it defines SDL's application callbacks. It mirrors
 * SDLPP_MAIN, but reaches the SDL application through detail::application_access, because
 * neutrino::application inherits the SDL machinery privately (roadmap P1.7) and the
 * derived-to-base conversion SDLPP_MAIN relies on is therefore not accessible from here.
 */

#include <neutrino/application.hh>
#include <sdlpp/app/entry_point.hh>

#define NEUTRINO_MAIN(AppClass)                                                                   \
    static AppClass g_neutrino_app_instance;                                                      \
                                                                                                  \
    extern "C" SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {               \
        auto* app = ::neutrino::detail::application_access::base(g_neutrino_app_instance);        \
        ::sdlpp::detail::g_app = app; /* sdlpp-ok */                                              \
        *appstate = app;                                                                          \
        return ::sdlpp::detail::app_init(app, argc, argv); /* sdlpp-ok */                         \
    }                                                                                             \
                                                                                                  \
    extern "C" SDL_AppResult SDL_AppIterate(void* appstate) {                                     \
        return ::sdlpp::detail::app_iterate(                                    /* sdlpp-ok */    \
            static_cast <::sdlpp::abstract_application*>(appstate));           /* sdlpp-ok */    \
    }                                                                                             \
                                                                                                  \
    extern "C" SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {                     \
        return ::sdlpp::detail::app_handle_event(                               /* sdlpp-ok */    \
            static_cast <::sdlpp::abstract_application*>(appstate), event);     /* sdlpp-ok */    \
    }                                                                                             \
                                                                                                  \
    extern "C" void SDL_AppQuit(void* appstate, SDL_AppResult) {                                  \
        ::sdlpp::detail::app_quit(static_cast <::sdlpp::abstract_application*>(appstate)); /* sdlpp-ok */ \
    }
