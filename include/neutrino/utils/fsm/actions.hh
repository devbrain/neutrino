//
// Created by igor on 23/09/2021.
//

#ifndef INCLUDE_NEUTRINO_UTILS_FSM_ACTIONS_HH
#define INCLUDE_NEUTRINO_UTILS_FSM_ACTIONS_HH

#include <variant>
#include <type_traits>

namespace neutrino::fsm {
  // --------------------------------------------------------------
  // Action types
  // --------------------------------------------------------------
  struct nothing {};

  template <typename State>
  struct transition_to {
    using to = State;
  };

  template <typename ... States>
  struct maybe {
    maybe()
    : options{} {}

    maybe(nothing)
    : options{} {}

    template <class T>
    maybe(transition_to<T>)
    : options((transition_to<T>*) nullptr) {}

    std::variant<std::monostate, std::add_pointer_t<transition_to<States>>...> options;
  };

  template <typename ... States>
  struct one_of {

    template <class T>
    one_of(transition_to<T>)
        : options((transition_to<T>*) nullptr) {}

    std::variant<std::add_pointer_t<transition_to<States>>...> options;
  };



  // --------------------------------------------------------------
  // Handler types
  // --------------------------------------------------------------

  template <typename Action>
  struct by_default {
    template <typename Event>
    Action handle (const Event&) const {
      return Action{};
    }
  };

  template <>
  struct by_default<nothing> {
    template <typename Event>
    nothing handle (const Event&) const {
      return {};
    }
  };

  template <>
  struct by_default<void> {
    template <typename Event>
    void handle (const Event&) const {
    }
  };

  template <typename Event, typename Action>
  struct on
  {
    Action handle(const Event&) const
    {
      return Action{};
    }
  };

  template <typename Event>
  struct on<Event, nothing>
  {
    nothing handle(const Event&) const
    {
      return {};
    }
  };

  template <typename Event>
  struct on<Event, void>
  {
    void handle(const Event&) const;
  };

  template <typename... Handlers>
  struct will : Handlers...
  {
    using Handlers::handle...;
  };
}

#endif //INCLUDE_NEUTRINO_UTILS_FSM_ACTIONS_HH
