//
// Created by igor on 07/10/2021.
//

#include "systems_manager.hh"

namespace neutrino::kernel {

  void systems_manager::add(std::unique_ptr<base_input_system> sys) {
    m_input_system = std::move(sys);
  }

  void systems_manager::add(std::unique_ptr<video_system> sys) {
    m_video_system = std::move(sys);
  }

  void systems_manager::add(std::unique_ptr<system> sys) {
    m_systems.push_back (std::move(sys));
  }

  base_input_system* systems_manager::get_input_system() {
    return m_input_system.get();
  }

  video_system* systems_manager::get_video_system() {
    return m_video_system.get();
  }

  void systems_manager::on_terminating () {
    m_input_system->on_terminating();
  }
  void systems_manager::on_low_memory () {
    m_input_system->on_low_memory();
  }
  void systems_manager::on_will_enter_background () {
    m_input_system->on_will_enter_background();
  }
  void systems_manager::on_in_background () {
    m_input_system->on_in_background();
  }

  void systems_manager::on_in_foreground () {
    m_input_system->on_in_foreground();
  }

  void systems_manager::on_input_focus_changed (bool keyboard_focus, bool mouse_focus) {
    m_input_system->on_input_focus_changed (keyboard_focus, mouse_focus);
  }

  void systems_manager::on_visibility_change (bool is_visible) {
    m_input_system->on_visibility_change (is_visible);
  }

  void systems_manager::on_keyboard_input (const events::keyboard& ev) {
    m_input_system->on_keyboard_input (ev);
  }

  void systems_manager::on_pointer_input (const events::pointer& ev) {
    m_input_system->on_pointer_input (ev);
  }

  void systems_manager::on_paused(bool paused) {
    for (auto& sys :m_systems) {
      sys->on_paused(paused);
    }
  }

  void systems_manager::update(std::chrono::milliseconds ms) {
    for (auto& sys :m_systems) {
      sys->update(ms);
    }
    m_video_system->update();
  }

  void systems_manager::present() {
    for (auto& sys :m_systems) {
      sys->present();
    }
    m_video_system->present();
  }

  static systems_manager* s_instance = nullptr;

  systems_manager* get_systems_manager() {
    if (!s_instance) {
      s_instance = new systems_manager;
    }
    return s_instance;
  }

  void release_systems_manager() {
    if (s_instance) {
      delete s_instance;
      s_instance = nullptr;
    }
  }
}
