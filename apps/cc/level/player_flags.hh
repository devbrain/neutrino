//
// Created by igor on 9/26/24.
//

#ifndef NEUTRINO_APPS_CC_LEVEL_PLAYER_FLAGS_HH_
#define NEUTRINO_APPS_CC_LEVEL_PLAYER_FLAGS_HH_

#include <cstdint>

inline constexpr uint32_t PLAYER_FLAGS_NONE           = 0;
inline constexpr uint32_t PLAYER_FLAGS_MOVING_LEFT    = 1;
inline constexpr uint32_t PLAYER_FLAGS_MOVING_RIGHT   = 2;
inline constexpr uint32_t PLAYER_FLAGS_MOVING_IN_JUMP = 4;
inline constexpr uint32_t PLAYER_FLAGS_MOVING_IN_FIRE = 8;

#endif
