//
// Created by igor on 12/06/2021.
//

#include "events_mapper.hh"
#include "scancode_mapper.hh"

namespace neutrino::hal::events {
#define d_TEST_KEY_MODE(SDL_F, F1)                  \
    if (x & (uint16_t)sdl::keymod::SDL_F) {         \
        return key_mod_t::SDL_F | key_mod_t::F1;    \
    }

  static key_mod_t map_key_mode (uint16_t x) {
    d_TEST_KEY_MODE(LALT, ALT)
    d_TEST_KEY_MODE(RALT, ALT)
    d_TEST_KEY_MODE(LCTRL, CTRL)
    d_TEST_KEY_MODE(RCTRL, CTRL)
    d_TEST_KEY_MODE(LSHIFT, SHIFT)
    d_TEST_KEY_MODE(RSHIFT, SHIFT)
    d_TEST_KEY_MODE(LGUI, GUI)
    d_TEST_KEY_MODE(RGUI, GUI)

    if (x & (uint16_t) sdl::keymod::MODE) {
      return key_mod_t::MODE;
    }
    if (x & (uint16_t) sdl::keymod::NUM) {
      return key_mod_t::NUM;
    }
    if (x & (uint16_t) sdl::keymod::CAPS) {
      return key_mod_t::CAPS;
    }
    return key_mod_t::NONE;
  }

  // -----------------------------------------------------------------------------------
  keyboard map_event (const sdl::events::keyboard& kv) {
    return {kv.pressed, map_scancode (kv.scan_code), map_key_mode (kv.key_mod)};
  }
  // -----------------------------------------------------------------------------------
#define D_MAP_MB(X) case sdl::events::mousebutton::X : return pointer_button_t::X;

  static pointer_button_t map_pointer_button (const sdl::events::mousebutton& x) {
    switch (x) {
      D_MAP_MB(LEFT)
      D_MAP_MB(RIGHT)
      D_MAP_MB(MIDDLE)
      D_MAP_MB(X1)
      D_MAP_MB(X2)
      default:
        return pointer_button_t::WHEEL;
    }
  }

  // -----------------------------------------------------------------------------------
  pointer map_event (const sdl::events::mouse_button& m) {
    return pointer (m.mouse_id, map_pointer_button (m.button), m.pressed, coords_t{m.x, m.y});
  }

  pointer map_event (const sdl::events::mouse_motion& m) {
    return pointer (m.mouse_id,
                    map_pointer_button (m.button),
                    false,
                    coords_t{m.x, m.y},
                    coords_t{m.xrel, m.yrel}
    );
  }

  pointer map_event (const sdl::events::mouse_wheel& m) {
    return pointer (m.mouse_id,
                    pointer_button_t::WHEEL,
                    true,
                    coords_t{m.x, m.y}
    );
  }

  pointer map_event (const sdl::events::touch_device_button& m) {
    return pointer (map_pointer_button (m.button),
                    false,
                    coords_t{m.x, m.y}
    );
  }

  pointer map_event (const sdl::events::touch_device_motion& m) {
    return pointer (map_pointer_button (m.button),
                    false,
                    coords_t{m.x, m.y},
                    coords_t{m.xrel, m.yrel}
    );
  }

  pointer map_event (const sdl::events::touch_device_wheel& m) {
    return pointer (pointer_button_t::WHEEL,
                    true,
                    coords_t{m.x, m.y}
    );
  }
}