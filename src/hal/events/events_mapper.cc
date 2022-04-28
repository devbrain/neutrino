//
// Created by igor on 12/06/2021.
//

#include "events_mapper.hh"
#include "scancode_mapper.hh"

namespace neutrino::hal::events {

#define d_TEST_KEY_MODE(SDL_F)                \
    if (x & (uint16_t)sdl::keymod::SDL_F) {   \
        return key_mod_t::SDL_F ;             \
    }

  static key_mod_t map_key_mode (uint16_t x) {
    d_TEST_KEY_MODE(LALT)
    d_TEST_KEY_MODE(RALT)
    d_TEST_KEY_MODE(LCTRL)
    d_TEST_KEY_MODE(RCTRL)
    d_TEST_KEY_MODE(LSHIFT)
    d_TEST_KEY_MODE(RSHIFT)
    d_TEST_KEY_MODE(LGUI)
    d_TEST_KEY_MODE(RGUI)

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

  static pointer_button_t map_pointer_button (uint32_t x) {
    pointer_button_t ret = pointer_button_t::NONE;
    if (x & SDL_BUTTON_LMASK) {
      ret |= pointer_button_t::LEFT;
    }
    if (x & SDL_BUTTON_RMASK) {
      ret |= pointer_button_t::RIGHT;
    }
    if (x & SDL_BUTTON_MMASK) {
      ret |= pointer_button_t::MIDDLE;
    }
    if (x & SDL_BUTTON_X1MASK) {
      ret |= pointer_button_t::X1;
    }
    if (x & SDL_BUTTON_X2MASK) {
      ret |= pointer_button_t::X2;
    }
    return ret;
  }
  // -----------------------------------------------------------------------------------
  pointer map_event (const sdl::events::mouse_button& m) {
    return {m.mouse_id, map_pointer_button (m.button), m.pressed ? pointer_state_t::PRESSED : pointer_state_t::RELEASED, coords_t{m.x, m.y}};
  }

  pointer map_event (const sdl::events::mouse_motion& m) {
    return {m.mouse_id,
            map_pointer_button (m.state),
                    pointer_state_t::MOTION,
                    coords_t{m.x, m.y},
                    coords_t{m.xrel, m.yrel}
    };
  }

  pointer map_event (const sdl::events::mouse_wheel& m) {
    return {m.mouse_id,
                    pointer_button_t::WHEEL,
                    pointer_state_t::PRESSED,
                    coords_t{m.x, m.y}
    };
  }

  pointer map_event (const sdl::events::touch_device_button& m) {
    return {map_pointer_button (m.button),
                    pointer_state_t::PRESSED,
                    coords_t{m.x, m.y}
    };
  }

  pointer map_event (const sdl::events::touch_device_motion& m) {
    return {map_pointer_button (m.button),
            pointer_state_t::MOTION,
                    coords_t{m.x, m.y},
                    coords_t{m.xrel, m.yrel}
    };
  }

  pointer map_event (const sdl::events::touch_device_wheel& m) {
    return {pointer_button_t::WHEEL,
            pointer_state_t::PRESSED,
                    coords_t{m.x, m.y}
    };
  }
}