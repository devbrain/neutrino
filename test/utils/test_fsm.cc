//
// Created by igor on 22/09/2021.
//

#include <doctest/doctest.h>
#include <neutrino/utils/fsm/fsm.hh>

using namespace neutrino::fsm;

struct open_event {
};

struct close_event {
};

struct closed_state;
struct open_state;

struct door_data {

  door_data () {
    reinit ();
  }

  void reinit () {
    handle_open_event = false;
    handle_close_event = false;
    leave_open_event = false;
    leave_close_event = false;
    enter_open_event = false;
  }

  bool handle_open_event {false};
  bool handle_close_event {false};
  bool leave_open_event {false};
  bool leave_close_event {false};
  bool enter_open_event {false};
};

struct closed_state {
  explicit closed_state (door_data& data)
      : m_data{data} {
  }

  door_data& m_data;

  [[nodiscard]] transition_to<open_state> handle (const open_event&) const {
    m_data.handle_open_event = true;
    // std::cout << "Opening the door..." << std::endl;
    return {};
  }

  void on_leave (const open_event&) {
    m_data.leave_open_event = true;
  }

  void handle (const close_event&) const {
    m_data.handle_close_event = true;
    // std::cout << "Cannot close. The door is already closed!" << std::endl;
  }

};

struct open_state {
  explicit open_state (door_data& data)
  : m_data{data} {
  }

  door_data& m_data;
  void handle (const open_event&) const {
    m_data.handle_open_event = true;
   // std::cout << "Cannot open. The door is already open!" << std::endl;
  }

  [[nodiscard]] transition_to<closed_state> handle (const close_event&) const {
    m_data.handle_close_event = true;
    // std::cout << "Closing the door..." << std::endl;
    return {};
  }

  void on_leave(const close_event&) {
    m_data.leave_close_event = true;
  }

  void on_enter(const open_event&) {
    m_data.enter_open_event = true;
  }
};
// ---------------------------------------------------------------------------------------------------------
TEST_SUITE("FSM tests") {
  TEST_CASE("fsm - trivial test") {
    door_data csdata;
    door_data osdata;
    state_machine<closed_state, open_state> door{closed_state{csdata}, open_state{osdata}};

    door.handle (open_event{});

    REQUIRE(osdata.enter_open_event);
    REQUIRE(csdata.handle_open_event);
    REQUIRE(csdata.leave_open_event);
    osdata.reinit ();
    csdata.reinit ();

    door.handle (close_event{});

    REQUIRE(!csdata.handle_close_event);
    REQUIRE(osdata.handle_close_event);
    REQUIRE(osdata.leave_close_event);
    osdata.reinit ();
    csdata.reinit ();

    door.handle (close_event{});
    REQUIRE(csdata.handle_close_event);
    osdata.reinit ();
    csdata.reinit ();

    door.handle (open_event{});
    REQUIRE(osdata.enter_open_event);
    REQUIRE(csdata.handle_open_event);
    REQUIRE(csdata.leave_open_event);
  }
}
