//
// Created by igor on 03/07/2021.
//

#include <neutrino/engine/application.hh>
#include <neutrino/engine/scene_manager.hh>
#include "neutrino/engine/scene_window.hh"

namespace neutrino::engine {
  struct application::impl {
    impl (std::unique_ptr<application_monitor> amonitor)
        : monitor (std::move (amonitor)) {

    }

    std::unique_ptr<application_monitor> monitor;

  };
  // ----------------------------------------------------------------------------------------------------------------------------------------------

  application::application (std::unique_ptr<application_monitor> monitor)
      : m_pimpl (spimpl::make_unique_impl<impl> (std::move (monitor))) {
    scene_manager::instance ().attach (this);
  }

  void application::on_terminating () {
    if (m_pimpl->monitor) {
      m_pimpl->monitor->on_terminating ();
    }
  }
  void application::on_low_memory () {
    if (m_pimpl->monitor) {
      m_pimpl->monitor->on_low_memory ();
    }
  }

  void application::on_will_enter_background () {
    if (m_pimpl->monitor) {
      m_pimpl->monitor->on_will_enter_background ();
    }
  }
  void application::on_in_background () {
    if (m_pimpl->monitor) {
      m_pimpl->monitor->on_in_background ();
    }
  }
  void application::on_in_foreground () {
    if (m_pimpl->monitor) {
      m_pimpl->monitor->on_in_foreground ();
    }
  }

  void application::update (std::chrono::milliseconds ms) {
    scene_manager::instance ().update (ms);
  }

  void application::on_event (const events::quit &) {
    this->quit ();
  }
}
