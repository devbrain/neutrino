//
// Created by igor on 05/06/2020.
//

#ifndef NEUTRINO_SDL_EVENTS_DISPATCHER_HH
#define NEUTRINO_SDL_EVENTS_DISPATCHER_HH

#include <variant>


#include <neutrino/utils/mp/typelist.hh>
#include <hal/sdl/events/system_events.hh>

namespace neutrino::sdl
{
    inline
    events::event_t map_event(const SDL_Event& e)
    {
#define d_EV_RUN(EVTYPE, MEMBER)                                                                     \
                return {EVTYPE(e.MEMBER)}

#define d_EV_RUN0(EVTYPE)                                                                            \
                return EVTYPE{}

#define d_DEFINE_EV_MAPPER(EVTYPE, MEMBER, MEMBERID)                                                \
    case MEMBERID:                                                                                  \
            return {EVTYPE(e.MEMBER)}                                                                \

#define d_DEFINE_EV_MAPPER2(EVTYPE, MEMBER, MEMBERID, MEMBERID2)                                    \
    case MEMBERID2:                                                                                 \
            d_EV_RUN(EVTYPE, MEMBER);                                                               \
            break;                                                                                  \
    case MEMBERID:                                                                                  \
            d_EV_RUN(EVTYPE, MEMBER)                                                                \

#define d_DEFINE_EV_MAPPER0(EVTYPE,  MEMBERID)                                                      \
    case MEMBERID:                                                                                  \
            d_EV_RUN0(EVTYPE)                                                                       \

        switch (e.type)
        {
            d_DEFINE_EV_MAPPER2(events::keyboard, key, SDL_KEYDOWN, SDL_KEYUP);

            case SDL_WINDOWEVENT:
                switch (e.window.event)
                {
                    d_DEFINE_EV_MAPPER(events::window_shown, window, SDL_WINDOWEVENT_SHOWN);
                    d_DEFINE_EV_MAPPER(events::window_hidden, window, SDL_WINDOWEVENT_HIDDEN);
                    d_DEFINE_EV_MAPPER(events::window_exposed, window, SDL_WINDOWEVENT_EXPOSED);
                    d_DEFINE_EV_MAPPER(events::window_moved, window, SDL_WINDOWEVENT_MOVED);
                    d_DEFINE_EV_MAPPER(events::window_resized, window, SDL_WINDOWEVENT_RESIZED);
                    d_DEFINE_EV_MAPPER(events::window_minimized, window, SDL_WINDOWEVENT_MINIMIZED);
                    d_DEFINE_EV_MAPPER(events::window_maximized, window, SDL_WINDOWEVENT_MAXIMIZED);
                    d_DEFINE_EV_MAPPER(events::window_restored, window, SDL_WINDOWEVENT_RESTORED);
                    d_DEFINE_EV_MAPPER(events::window_mouse_entered, window, SDL_WINDOWEVENT_ENTER);
                    d_DEFINE_EV_MAPPER(events::window_mouse_leaved, window, SDL_WINDOWEVENT_LEAVE);
                    d_DEFINE_EV_MAPPER(events::window_focus_gained, window, SDL_WINDOWEVENT_FOCUS_GAINED);
                    d_DEFINE_EV_MAPPER(events::window_focus_lost, window, SDL_WINDOWEVENT_FOCUS_LOST);
                    d_DEFINE_EV_MAPPER(events::window_close, window, SDL_WINDOWEVENT_CLOSE);
                    default:
                        return {};
                }
            d_DEFINE_EV_MAPPER(events::text_editing, edit, SDL_TEXTEDITING);
            d_DEFINE_EV_MAPPER(events::text_input, text, SDL_TEXTINPUT);

            case SDL_MOUSEMOTION:
                if (e.motion.which != SDL_TOUCH_MOUSEID)
                {
                        d_EV_RUN(events::mouse_motion, motion);
                } else {
                        d_EV_RUN(events::touch_device_motion, motion);
                }


            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                if (e.button.which != SDL_TOUCH_MOUSEID)
                {
                        d_EV_RUN(events::mouse_button, button);
                }
                else
                {
                        d_EV_RUN(events::touch_device_button, button);
                }


             case SDL_MOUSEWHEEL:
                 if (e.wheel.which != SDL_TOUCH_MOUSEID)
                 {
                         d_EV_RUN(events::mouse_wheel, wheel);
                 } else
                 {
                         d_EV_RUN(events::touch_device_wheel, wheel);
                 }


            d_DEFINE_EV_MAPPER(events::joystick_axis, jaxis, SDL_JOYAXISMOTION);
            d_DEFINE_EV_MAPPER(events::joystick_ball, jball, SDL_JOYBALLMOTION);

            d_DEFINE_EV_MAPPER2(events::joystick_button, jbutton, SDL_JOYBUTTONUP, SDL_JOYBUTTONDOWN);
            d_DEFINE_EV_MAPPER(events::joystick_hat, jhat, SDL_JOYHATMOTION);
            d_DEFINE_EV_MAPPER(events::user, user, SDL_USEREVENT);

            d_DEFINE_EV_MAPPER0(events::terminating, SDL_APP_TERMINATING);
            d_DEFINE_EV_MAPPER0(events::low_memory, SDL_APP_LOWMEMORY);
            d_DEFINE_EV_MAPPER0(events::will_enter_background, SDL_APP_WILLENTERBACKGROUND);
            d_DEFINE_EV_MAPPER0(events::in_background, SDL_APP_DIDENTERBACKGROUND);
            d_DEFINE_EV_MAPPER0(events::will_enter_foreground, SDL_APP_WILLENTERFOREGROUND);
            d_DEFINE_EV_MAPPER0(events::in_foreground, SDL_APP_DIDENTERFOREGROUND);
            d_DEFINE_EV_MAPPER0(events::quit, SDL_QUIT);
        }

        return {};
    }
}


#endif
