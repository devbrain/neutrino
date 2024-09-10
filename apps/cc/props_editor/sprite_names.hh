//
// Created by igor on 9/8/24.
//

#ifndef SPRITE_NAMES_HH
#define SPRITE_NAMES_HH

#include <array>
#include <bsw/macros.hh>

enum sprite_names {
  SPRITE_DOOR,
};


template <int Name>
struct sprite_sequence {
  static constexpr bool is_sequence = false;
};

#define PRINT_SEQ_(i, x) x,
#define SPRITE_SEQ(SPRITE, SUB_SPRITE, ...)                                                                      \
enum SUB_SPRITE { FOR_EACH_COUNT(PRINT_SEQ_, __VA_ARGS__) };                                                     \
template <>                                                                                                      \
struct sprite_sequence<SPRITE> {                                                                                 \
  static constexpr std::array<SUB_SPRITE, PP_NARG(__VA_ARGS__)> seq = {FOR_EACH_COUNT(PRINT_SEQ_, __VA_ARGS__)}; \
  using seq_type = SUB_SPRITE;                                                                                   \
  static constexpr bool is_sequence = true;                                                                      \
}

SPRITE_SEQ(SPRITE_DOOR, door_states, DOOR_OPENS, DOOR_CHECKED);

#endif
