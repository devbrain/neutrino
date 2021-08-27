//
// Created by igor on 03/07/2021.
//

#ifndef NEUTRINO_ENGINE_SCENE_HH
#define NEUTRINO_ENGINE_SCENE_HH

#include <chrono>
#include <neutrino/engine/events.hh>

namespace neutrino::engine {
  class scene {
    public:
      explicit scene (int scene_id);
      virtual ~scene ();

      [[nodiscard]] int id () const noexcept;

      virtual void on_enter () = 0;
      virtual void on_exit () = 0;

      virtual void update (std::chrono::milliseconds ms) = 0;
      virtual void on_input_focus_changed (bool keyboard_focus, bool mouse_focus) = 0;
      virtual void on_visibility_change (bool is_visible) = 0;
      virtual void on_keyboard_input (const events::keyboard &ev) = 0;
      virtual void on_pointer_input (const events::pointer &ev) = 0;
    private:
      int m_id;
  };
}

#endif
