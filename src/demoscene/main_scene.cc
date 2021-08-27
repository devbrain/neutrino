//
// Created by igor on 03/07/2021.
//

#include <sstream>
#include <neutrino/engine/scene_manager.hh>
#include <neutrino/engine/application.hh>
#include "main_scene.hh"

namespace neutrino::demoscene {
  main_scene::main_scene (demoscene::scene *owner)
      : engine::scene (0),
        m_owner (owner),
        m_show_fps (false),
        m_current_fps (0) {
    hal::get_application ()->attach (this);
  }

  void main_scene::on_event (const engine::events::current_fps &e) {
    m_current_fps = e.value;
    show_fps ();
  }

  void main_scene::on_enter () {
    auto *gc = m_owner->m_vga.get ();
    m_owner->init (*gc);
  }

  void main_scene::on_exit () {

  }

  void main_scene::update ([[maybe_unused]] std::chrono::milliseconds ms) {
    auto *gc = m_owner->m_vga.get ();
    m_owner->effect (*gc);
    gc->present ();
  }

  void main_scene::on_input_focus_changed ([[maybe_unused]] bool keyboard_focus, [[maybe_unused]] bool mouse_focus) {

  }

  void main_scene::on_visibility_change ([[maybe_unused]] bool is_visible) {

  }

  void main_scene::on_keyboard_input (const engine::events::keyboard &ev) {
    if (ev.pressed) {
      if (ev.code == engine::events::scan_code_t::ESCAPE) {
        engine::scene_manager::instance ().notify (engine::events::quit{});
      }
      else {
        if (ev.code == engine::events::scan_code_t::RETURN && (ev.mod & engine::events::key_mod_t::ALT)) {
          engine::scene_manager::instance ().notify (engine::events::full_screen{});
        }
        else {
          if (ev.code == engine::events::scan_code_t::F) {
            m_show_fps ^= true;
            show_fps ();
          }
        }
      }

    }
  }

  void main_scene::on_pointer_input ([[maybe_unused]] const engine::events::pointer &ev) {

  }

  void main_scene::show_fps () {
    std::ostringstream os;
    if (m_show_fps) {
      os << "FPS: " << m_current_fps;
    }
    m_owner->title (os.str ());
  }
}