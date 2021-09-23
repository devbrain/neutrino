//
// Created by igor on 23/09/2021.
//

#ifndef INCLUDE_NEUTRINO_UTILS_FSM_ACTION_TRAITS_HH
#define INCLUDE_NEUTRINO_UTILS_FSM_ACTION_TRAITS_HH

#include <neutrino/utils/fsm/actions.hh>

namespace neutrino::fsm::traits {
  // ----------------------------------------------------------------------------------
  // Checks if action is nothing
  // ----------------------------------------------------------------------------------
  template <typename T>
  struct is_nothing {
    constexpr static bool value = false;
  };

  template <>
  struct is_nothing<nothing> {
    constexpr static bool value = true;
  };

  template <>
  struct is_nothing<void> {
    constexpr static bool value = true;
  };

  template <>
  struct is_nothing<std::monostate> {
    constexpr static bool value = true;
  };

  template <typename T>
  inline constexpr bool is_nothing_v = is_nothing<T>::value;

  // ----------------------------------------------------------------------------------
  template <typename T>
  struct is_transition {
    constexpr static bool value = false;
  };

  template <typename T>
  struct is_transition<transition_to < T>> {
    constexpr static bool value = true;
    constexpr static bool complex = false;
  };

  template <typename ... States>
  struct is_transition<one_of < States...>>{
    constexpr static bool value = true;
    constexpr static bool complex = true;
  };

  template <typename ... States>
  struct is_transition<maybe < States...>>{
    constexpr static bool value = true;
    constexpr static bool complex = true;
  };

  template <typename T>
  inline constexpr bool is_transition_v = is_transition<T>::value;

  template <typename T>
  inline constexpr bool is_complex_transition_v = is_transition<T>::complex;
}

#endif //INCLUDE_NEUTRINO_UTILS_FSM_ACTION_TRAITS_HH
