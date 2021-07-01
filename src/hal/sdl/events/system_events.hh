//
// Created by igor on 04/06/2020.
//

#ifndef SDLPP_SYSTEM_EVENTS_HH
#define SDLPP_SYSTEM_EVENTS_HH

#include <neutrino/utils/mp/typelist.hh>
#include <hal/sdl/events/event_types.hh>

namespace neutrino::sdl::events
{
    using system_event = SDL_Event;

    namespace detail
    {
        class window_event
        {
        public:
            const uint32_t window_id;
        protected:
            explicit window_event(uint32_t wid)
                    : window_id(wid)
            {
            }
        };
    } // ns detail

    class keyboard : public detail::window_event
    {
    public:
        const bool pressed;
        const bool repeat;
        const scancode scan_code;
        const keycode key_code;
        const uint16_t key_mod;

        explicit keyboard(const SDL_KeyboardEvent& e)
                : detail::window_event(e.windowID),
                  pressed(e.state == SDL_PRESSED),
                  repeat(e.repeat > 0),
                  scan_code(static_cast <scancode> (e.keysym.scancode)),
                  key_code(static_cast <keycode> (e.keysym.sym)),
                  key_mod(e.keysym.mod)
        {
        }
    };

#define d_DEFINE_NO_MEMBERS_EVENT_WIN(NAME)                         \
  class NAME : public detail::window_event                          \
  {                                                                 \
    public: explicit NAME (const SDL_WindowEvent& e)                \
      : detail::window_event (e.windowID) {}                        \
  }

    d_DEFINE_NO_MEMBERS_EVENT_WIN(window_shown);
    d_DEFINE_NO_MEMBERS_EVENT_WIN(window_hidden);
    d_DEFINE_NO_MEMBERS_EVENT_WIN(window_exposed);
    d_DEFINE_NO_MEMBERS_EVENT_WIN(window_minimized);
    d_DEFINE_NO_MEMBERS_EVENT_WIN(window_maximized);
    d_DEFINE_NO_MEMBERS_EVENT_WIN(window_restored);
    d_DEFINE_NO_MEMBERS_EVENT_WIN(window_mouse_entered);
    d_DEFINE_NO_MEMBERS_EVENT_WIN(window_mouse_leaved);
    d_DEFINE_NO_MEMBERS_EVENT_WIN(window_focus_gained);
    d_DEFINE_NO_MEMBERS_EVENT_WIN(window_focus_lost);
    d_DEFINE_NO_MEMBERS_EVENT_WIN(window_close);

#define d_DEFINE_NO_MEMBERS_EVENT(NAME)         \
  class NAME                                    \
  {                                             \
    public: NAME () {}                          \
  }

// urgent events
    d_DEFINE_NO_MEMBERS_EVENT(terminating);
    d_DEFINE_NO_MEMBERS_EVENT(low_memory);
    d_DEFINE_NO_MEMBERS_EVENT(will_enter_background);
    d_DEFINE_NO_MEMBERS_EVENT(in_background);
    d_DEFINE_NO_MEMBERS_EVENT(will_enter_foreground);
    d_DEFINE_NO_MEMBERS_EVENT(in_foreground);
    d_DEFINE_NO_MEMBERS_EVENT(quit);

    class window_moved : public detail::window_event
    {
    public:
        unsigned x;
        unsigned y;

        explicit window_moved(const SDL_WindowEvent& e)
                : detail::window_event(e.windowID),
                  x(static_cast <unsigned> (e.data1)),
                  y(static_cast <unsigned> (e.data2))
        {
        }
    };

// ===========================================================================
    class window_resized : public detail::window_event
    {
    public:
        unsigned w;
        unsigned h;

        explicit window_resized(const SDL_WindowEvent& e)
                : detail::window_event(e.windowID),
                  w(static_cast <unsigned> (e.data1)),
                  h(static_cast <unsigned> (e.data2))
        {
        }
    };

// ======================================================================
    class text_editing : public detail::window_event
    {

    public:
        static constexpr size_t MAX_TEXT_LENGTH = SDL_TEXTEDITINGEVENT_TEXT_SIZE;

        const char* text;
        unsigned start;
        unsigned length;

        explicit text_editing(const SDL_TextEditingEvent& e)
                : detail::window_event(e.windowID),
                  text(e.text),
                  start(static_cast<unsigned>(e.start)),
                  length(static_cast<unsigned>(e.length))
        {
        }
    };

// ======================================================================
    class text_input : public detail::window_event
    {
    public:
        static constexpr size_t MAX_TEXT_LENGTH = SDL_TEXTEDITINGEVENT_TEXT_SIZE;

        const char* text;

        explicit text_input(const SDL_TextInputEvent& e)
                : detail::window_event(e.windowID),
                  text(e.text)
        {
        }
    };

// ======================================================================
    enum class mousebutton
    {

        LEFT = SDL_BUTTON_LEFT,
        RIGHT = SDL_BUTTON_RIGHT,
        MIDDLE = SDL_BUTTON_MIDDLE,
        X1 = SDL_BUTTON_X1,
        X2 = SDL_BUTTON_X2
    };
    typedef Uint32 mouse_id_t;

    struct mouse_motion : public detail::window_event
    {
    public:
        mouse_id_t mouse_id;
        mousebutton button;
        int x;
        int y;
        int xrel;
        int yrel;

        explicit mouse_motion(const SDL_MouseMotionEvent& e)
                : detail::window_event(e.windowID),
                  mouse_id(static_cast <mouse_id_t> (e.which)),
                  button(static_cast <mousebutton> (e.state)),
                  x(e.x),
                  y(e.y),
                  xrel(e.xrel),
                  yrel(e.yrel)
        {
        }
    };

// ======================================================================
    struct touch_device_motion : public detail::window_event
    {
    public:
        mousebutton button;
        int x;
        int y;
        int xrel;
        int yrel;

        explicit touch_device_motion(const SDL_MouseMotionEvent& e)
                : detail::window_event(e.windowID),
                  button(static_cast <mousebutton> (e.state)),
                  x(e.x),
                  y(e.y),
                  xrel(e.xrel),
                  yrel(e.yrel)
        {
        }
    };

// ======================================================================
    class mouse_button : public detail::window_event
    {
    public:
        mouse_id_t mouse_id;
        mousebutton button;
        int x;
        int y;
        bool pressed;

        explicit mouse_button(const SDL_MouseButtonEvent& e)
                : detail::window_event(e.windowID),
                  mouse_id(static_cast <mouse_id_t> (e.which)),
                  button(static_cast <mousebutton> (e.button)),
                  x(e.x),
                  y(e.y),
                  pressed(e.state == SDL_PRESSED)
        {
        }
    };

// ======================================================================
    class touch_device_button : public detail::window_event
    {
    public:
        mousebutton button;
        int x;
        int y;
        bool pressed;

        explicit touch_device_button(const SDL_MouseButtonEvent& e)
                : detail::window_event(e.windowID),
                  button(static_cast <mousebutton> (e.button)),
                  x(e.x),
                  y(e.y),
                  pressed(e.state == SDL_PRESSED)
        {
        }
    };

// ======================================================================
    class mouse_wheel : public detail::window_event
    {
    public:
        mouse_id_t mouse_id;
        int x;
        int y;

        explicit mouse_wheel(const SDL_MouseWheelEvent& e)
                : detail::window_event(e.windowID),
                  mouse_id(static_cast <mouse_id_t> (e.which)),
                  x(e.x),
                  y(e.y)
        {
        }
    };

// ======================================================================
    class touch_device_wheel : public detail::window_event
    {
    public:
        int x;
        int y;

        explicit touch_device_wheel(const SDL_MouseWheelEvent& e)
                : detail::window_event(e.windowID),
                  x(e.x),
                  y(e.y)
        {
        }
    };
// ======================================================================
    using joystick_id = SDL_JoystickID;

    class joystick_axis
    {
    public:
        joystick_id joystick;
        uint8_t axis;
        signed short value;

        explicit joystick_axis(const SDL_JoyAxisEvent& e)
                : joystick(e.which),
                  axis(e.axis),
                  value(e.value)
        {
        }
    };

// ======================================================================
    class joystick_ball
    {
    public:
        joystick_id joystick;
        uint8_t ball;
        signed short xrel;
        signed short yrel;

        explicit joystick_ball(const SDL_JoyBallEvent& e)
                : joystick(e.which),
                  ball(e.ball),
                  xrel(e.xrel),
                  yrel(e.yrel)
        {
        }
    };

// ======================================================================
    class joystick_button
    {
    public:
        joystick_id joystick;
        Uint8 button;
        bool pressed;

        explicit joystick_button(const SDL_JoyButtonEvent& e)
                : joystick(e.which),
                  button(e.button),
                  pressed(e.state == SDL_PRESSED)
        {
        }
    };
// ======================================================================

    enum class joystick_hat_state
    {

        LEFTUP = SDL_HAT_LEFTUP,
        HAT_UP = SDL_HAT_UP,
        HAT_RIGHTUP = SDL_HAT_RIGHTUP,
        HAT_LEFT = SDL_HAT_LEFT,
        HAT_CENTERED = SDL_HAT_CENTERED,
        HAT_RIGHT = SDL_HAT_RIGHT,
        HAT_LEFTDOWN = SDL_HAT_LEFTDOWN,
        HAT_DOWN = SDL_HAT_DOWN,
        HAT_RIGHTDOWN = SDL_HAT_RIGHTDOWN
    };

    class joystick_hat
    {

    public:
        joystick_id joystick;
        Uint8 value;
        joystick_hat_state state;

        explicit joystick_hat(const SDL_JoyHatEvent& e)
                : joystick(e.which),
                  value(e.value),
                  state(static_cast <joystick_hat_state>(e.value))
        {
        }
    };

// ======================================================================
    struct user
    {
    public:

        explicit user(const SDL_UserEvent& u)
                : code(u.code),
                  data1(u.data1),
                  data2(u.data2)
        {

        }

        explicit user(int32_t code_)
                : code(code_),
                  data1(nullptr),
                  data2(nullptr)
        {
        }

        user(int32_t code_, void* d1, void* d2 = nullptr)
                : code(code_),
                  data1(d1),
                  data2(d2)
        {
        }

        int32_t code;
        void* data1;
        void* data2;
    };
    // ==================================================================================================
    using all_events_t = mp::type_list<
            keyboard,
            window_shown,
            window_hidden,
            window_exposed,
            window_minimized,
            window_maximized,
            window_restored,
            window_mouse_entered,
            window_mouse_leaved,
            window_focus_gained,
            window_focus_lost,
            window_close,
            terminating,
            low_memory,
            will_enter_background,
            in_background,
            will_enter_foreground,
            in_foreground,
            quit,
            window_moved,
            window_resized,
            text_editing ,
            text_input,
            mouse_motion,
            touch_device_motion,
            mouse_button ,
            touch_device_button,
            mouse_wheel,
            touch_device_wheel,
            joystick_axis,
            joystick_ball,
            joystick_button,
            joystick_hat,
            user>;


    // ============================================================================================================
    using event_t = mp::type_list_to_variant_t<mp::type_list_prepend_t<std::monostate, all_events_t>>;
}
#endif //SDLPP_SYSTEM_EVENTS_HH
