//
// Created by igor on 08/06/2021.
//

#ifndef NEUTRINO_EVENTS_EVENTS_HH
#define NEUTRINO_EVENTS_EVENTS_HH


#include <neutrino/utils/mp/typelist.hh>
#include <neutrino/hal/events/scancodes.hh>
#include <neutrino/math/point.hh>
#include <bitflags/bitflags.hpp>
#include <optional>

namespace neutrino::hal::events {


    struct current_fps {
        unsigned value;
    };

    BEGIN_BITFLAGS(key_mod_t)
        FLAG(LSHIFT)
        FLAG(RSHIFT)
        FLAG(LCTRL)
        FLAG(RCTRL)
        FLAG(LALT)
        FLAG(RALT)
        FLAG(LGUI)
        FLAG(RGUI)
        FLAG(NUM)
        FLAG(CAPS)
        FLAG(MODE)
        FLAG(CTRL)
        FLAG(ALT)
        FLAG(SHIFT)
        FLAG(GUI)
        FLAG(NONE)
    END_BITFLAGS(key_mod_t)

    struct keyboard {
        bool pressed;
        scan_code_t code;
        key_mod_t mod;

        keyboard(bool is_pressed, scan_code_t key_code, key_mod_t key_mod)
        : pressed(is_pressed), code(key_code), mod(key_mod) {}

    };

    using coords_t = neutrino::math::point2d;

    enum class pointer_button_t {
        LEFT,
        RIGHT,
        MIDDLE,
        X1,
        X2,
        WHEEL
    };

    // Aggregates mouse and touch events
    struct pointer {
        std::optional<uint32_t> device_id;
        pointer_button_t button;
        bool pressed;
        coords_t point;
        std::optional<coords_t> relative_motion;

        pointer(uint32_t mouse_device_id, pointer_button_t pointer_button, bool is_pressed,
                coords_t point_coord, coords_t relative_motion_coord)
                : device_id(mouse_device_id),
                button(pointer_button),
                pressed(is_pressed),
                point(point_coord),
                relative_motion(relative_motion_coord) {}

        pointer(pointer_button_t pointer_button, bool is_pressed,
                coords_t point_coord, coords_t relative_motion_coord)
                : button(pointer_button),
                  pressed(is_pressed),
                  point(point_coord),
                  relative_motion(relative_motion_coord) {}

        pointer(uint32_t mouse_device_id, pointer_button_t pointer_button, bool is_pressed,
                coords_t point_coord)
                : device_id(mouse_device_id),
                  button(pointer_button),
                  pressed(is_pressed),
                  point(point_coord) {}

        pointer(pointer_button_t pointer_button, bool is_pressed,
                coords_t point_coord)
                : button(pointer_button),
                  pressed(is_pressed),
                  point(point_coord) {}
    };

    struct visibility_changed {
        bool visible;
    };

    struct window_resized {
        int w;
        int h;
    };

    struct before_window_closed {

    };

    struct input_focus_changed {
        bool keyboard_focus;
        bool mouse_focus;
    };

    using all_events_t = mp::type_list<
            current_fps,
            visibility_changed,
            window_resized,
            before_window_closed,
            input_focus_changed,
            keyboard,
            pointer>;

}

#endif
