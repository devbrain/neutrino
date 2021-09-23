//
// Created by igor on 22/09/2021.
//

#ifndef INCLUDE_NEUTRINO_UTILS_FSM_FSM_HH
#define INCLUDE_NEUTRINO_UTILS_FSM_FSM_HH

/**
 * FSM implementation
 * https://sii.pl/blog/implementing-a-state-machine-in-c17/
 */
#include <tuple>
#include <variant>
#include <neutrino/utils/fsm/actions.hh>
#include <neutrino/utils/fsm/action_traits.hh>

/**
 * every state should contain method handle(Event) and optional methods on_leave(Event), on_enter(Event)
 * every handle should return nothing/void or transition_to<State>
 */

namespace neutrino::fsm {

  template <typename ... States>
  class state_machine {
    public:
      state_machine () = default;

      explicit state_machine (States... states);

      template <typename Event>
      void handle (const Event& event);


    private:
      template <typename NewState, typename CurrentState, typename Event>
      void transition_to (CurrentState* state_ptr, const Event& cause);

      /*
       * SFINAE for on_leave
       */
      void leave (...) {
      }

      template <typename State, typename Event>
      auto leave (State* state, const Event& event) -> decltype (state->on_leave (event)) {
        return state->on_leave (event);
      }

      /*
       * SFINAE for on_enter
       */
      void enter (...) {
      }

      template <typename State, typename Event>
      auto enter (State* state, const Event& event) -> decltype (state->on_enter (event)) {
        return state->on_enter (event);
      }

    private:
      std::tuple<States...> m_states;
      std::variant<States* ...> m_current_state{&std::get<0> (m_states)};
  };

  // =======================================================================================
  // Implementation
  // =======================================================================================

  template <typename... States>
  state_machine<States...>::state_machine (States... states)
      : m_states{std::move (states)...} {
  }

  template <typename... States>
  template <typename NewState, typename CurrentState, typename Event>
  void state_machine<States...>::transition_to (CurrentState* state_ptr, const Event& cause) {
    if constexpr(!std::is_same_v<NewState, CurrentState>) {
      leave(state_ptr, cause);
      auto* new_state = &std::get<NewState> (m_states);
      m_current_state = new_state;
      enter (new_state, cause);
    }
  }

  template <typename... States>
  template <typename Event>
  void state_machine<States...>::handle (const Event& event) {

    auto pass_event_to_state = [&event, this] (auto state_ptr) {

      using transition_type = decltype (state_ptr->handle (event));

      if constexpr(!traits::is_nothing_v<transition_type>) {
        auto action = state_ptr->handle (event);
        static_assert (traits::is_transition_v<transition_type>, "Unsupported action type");
        if constexpr(!traits::is_complex_transition_v<transition_type>) {
          using new_state = typename transition_type::to;
          this->template transition_to<new_state> (state_ptr, event);
        } else {
          std::visit ([&event, this, state_ptr](auto act) {
            using act_t = std::remove_pointer_t<decltype (act)>;
            if constexpr (!traits::is_nothing_v<act_t>) {
              static_assert(!traits::is_complex_transition_v<act_t>, "Only simple transitions are supported in this context");
              using new_state = typename act_t::to;
              this->template transition_to<new_state> (state_ptr, event);
            }
          }, action.options);
        }
      } else {
          state_ptr->handle (event);
      }
    };
    std::visit (pass_event_to_state, m_current_state);
  }

}

#endif //INCLUDE_NEUTRINO_UTILS_FSM_FSM_HH
