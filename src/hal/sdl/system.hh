//
// Created by igor on 31/05/2020.
//

#ifndef NEUTRINO_SDL_SYSTEM_HH
#define NEUTRINO_SDL_SYSTEM_HH

#include <cstdint>

#include <hal/sdl/call.hh>
#include <hal/sdl/sdl2.hh>
#include <neutrino/utils/mp/all_same.hh>

namespace neutrino::sdl {
  enum class init_flags : uint32_t {
      TIMER = SDL_INIT_TIMER,
      AUDIO = SDL_INIT_AUDIO,
      VIDEO = SDL_INIT_VIDEO,
      JOYSTICK = SDL_INIT_JOYSTICK,
      HAPTIC = SDL_INIT_HAPTIC,
      GAMECONTROLLER = SDL_INIT_GAMECONTROLLER,
      EVENTS = SDL_INIT_EVENTS,
      SENSOR = SDL_INIT_SENSOR,
      NOPARACHUTE = SDL_INIT_NOPARACHUTE
  };

  class system {
    public:
      template <typename ... Args,
          typename std::enable_if_t<std::conjunction_v<std::is_same<Args, init_flags>...>, void *> = nullptr>
      explicit system (Args... flags);

      ~system () noexcept;
  };
} // ns sdl

// ====================================================================
// Implementation
// ====================================================================

namespace neutrino::sdl {

  template <typename ... Args,
      typename std::enable_if_t<std::conjunction_v<std::is_same<Args, init_flags>...>, void *>>
  system::system (Args... flags) {
    uint32_t f = (static_cast<std::uint32_t>(flags) | ... | 0u);
    if (f == 0) {
      f = SDL_INIT_EVERYTHING;
    }
    SAFE_SDL_CALL(SDL_Init, f);
  }
  // -----------------------------------------------------------------------------------------------
  inline
  system::~system () noexcept {
    SDL_Quit ();
  }
}

#endif
