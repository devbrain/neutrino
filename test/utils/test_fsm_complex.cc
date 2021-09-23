//
// Created by igor on 23/09/2021.
//

#include <doctest/doctest.h>
#include <neutrino/utils/fsm/fsm.hh>
#include <cstdint>
#include <iostream>

struct OpenEvent
{
};

struct CloseEvent
{
};

struct LockEvent
{
  uint32_t newKey;
};

struct UnlockEvent
{
  uint32_t key;
};

struct ClosedState;
struct OpenState;
class LockedState;

using namespace neutrino::fsm;

  struct ClosedState : public will<by_default<nothing>,
                     on<LockEvent, transition_to<LockedState>>,
                     on<OpenEvent, transition_to<OpenState>>>
{
};

struct OpenState : public will<by_default<nothing>,
                   on<CloseEvent, transition_to<ClosedState>>>
{
};

class LockedState : public by_default<nothing>
{
  public:
    using by_default::handle;

    LockedState(uint32_t key)
        : key(key)
    {
    }

    void on_enter(const LockEvent& e)
    {
      key = e.newKey;
    }

    maybe<ClosedState> handle(const UnlockEvent& e)
    {
      if (e.key == key) {
        return transition_to<ClosedState>{};
      }
      return nothing{};
    }

  private:
    uint32_t key;
};

using Door = state_machine<ClosedState, OpenState, LockedState>;

TEST_CASE("fsm test complex case") {
    Door door{ClosedState{}, OpenState{}, LockedState{0}};

    door.handle(LockEvent{1234});
    door.handle(UnlockEvent{2});
    door.handle(UnlockEvent{1234});
}

