//
// Created by igor on 07/10/2021.
//

#include <neutrino/kernel/application.hh>
#include "main_window.hh"
#include "hdi/kbd_mapper.hh"
#include "hdi/pointer_mapper.hh"

namespace neutrino {
  struct application::impl {
    impl()
        : window(nullptr),
          paused(false),
          renderer (nullptr),
          m_events(),
          m_kbd_mapper (m_events),
          m_pointer_mapper (m_events) {
    }

    ~impl() = default;

    std::unique_ptr<main_window> window;
    bool paused;
    hal::renderer* renderer;

    events_holder m_events;
    kbd_mapper m_kbd_mapper;
    pointer_mapper m_pointer_mapper;
  };

  neutrino::application::application () {
    m_pimpl = std::make_unique<application::impl>();
  }

  neutrino::application::~application () = default;

  void neutrino::application::execute () {
    auto app_descr = this->describe();
    m_pimpl->window = std::make_unique<main_window>(app_descr.get_main_window_description(), this);
    m_pimpl->window->show();
    m_pimpl->renderer = m_pimpl->window->renderer();
    this->init (*m_pimpl->renderer);
    this->run(static_cast<int>(app_descr.fps()));
    this->on_exit();
  }

  void neutrino::application::on_exit () {
  }

  void neutrino::application::on_terminating () {
    hal::application::on_terminating ();
  }

  void neutrino::application::on_low_memory () {
    hal::application::on_low_memory ();
  }

  void neutrino::application::on_will_enter_background () {
    hal::application::on_will_enter_background ();
  }

  void neutrino::application::on_in_background () {
    hal::application::on_in_background ();
  }

  void neutrino::application::on_in_foreground () {
    hal::application::on_in_foreground ();
  }

  void neutrino::application::on_window_resized ([[maybe_unused]] unsigned int new_w, [[maybe_unused]] unsigned int new_h) {
  }

  void neutrino::application::on_paused () {
  }

  void neutrino::application::on_resumed () {
  }

  void neutrino::application::close () {
    if (m_pimpl->window) {
      m_pimpl->window->close();
    }
  }

  void neutrino::application::toggle_fullscreen () {
    if (m_pimpl->window) {
      m_pimpl->window->toggle_fullscreen();
    }
  }

  void neutrino::application::update (std::chrono::milliseconds ms) {
    if (!m_pimpl->paused) {
      update_logic (ms);
    }
    m_pimpl->m_events.reset();
  }

  void neutrino::application::on_keyboard_input (const hal::events::keyboard& ev) {
    m_pimpl->m_kbd_mapper.handle_event (ev);
  }

  void neutrino::application::on_pointer_input (const hal::events::pointer& ev) {
    m_pimpl->m_pointer_mapper.handle_event (ev);
  }

  void application::do_draw_frame () {

    if (!m_pimpl->paused && m_pimpl->renderer) {
      this->draw_frame();
    }
  }

  void application::set_paused (bool v) {
    if (v) {
      if (!m_pimpl->paused) {
        m_pimpl->paused = true;
        this->on_paused();
      }
    } else {
      if (m_pimpl->paused) {
        m_pimpl->paused = false;
        this->on_resumed();
      }
    }
  }

  bool application::is_paused () const noexcept {
    return m_pimpl->paused;
  }

  input_config_base& application::input_config() {
    return m_pimpl->m_kbd_mapper;
  }

  events_holder& application::events() {
    return m_pimpl->m_events;
  }

  pointer_config_base& application::mouse_config() {
    return m_pimpl->m_pointer_mapper;
  }
}