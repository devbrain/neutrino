//
// Created by igor on 08/06/2021.
//

#include <neutrino/utils/observer.hh>

namespace neutrino::utils::detail {
  observer_monitor::~observer_monitor () {
    for (auto* m : m_holders) {
      m->destruct (this);
    }
  }

  void observer_monitor::_on_attached (monitors_holder* mh) {
    m_holders.insert (mh);
    on_subscribed ();
  }

  void observer_monitor::_on_detached (monitors_holder* mh) {
    m_holders.erase (mh);
    on_unsubscribed ();
  }

  void observer_monitor::on_subscribed () {
  }

  void observer_monitor::on_unsubscribed () {
  }

  // =======================================================================================
  void monitors_holder::insert (observer_monitor* obs) {
    m_monitors.insert (obs);
    obs->_on_attached (this);
  }

  void monitors_holder::remove (observer_monitor* obs) {
    m_monitors.erase (obs);
    obs->_on_detached (this);
  }

  void monitors_holder::destruct (observer_monitor* obs) {
    m_monitors.erase (obs);
  }

  monitors_holder::~monitors_holder () {
    for (auto* m : m_monitors) {
      m->_on_detached (this);
    }
  }
}