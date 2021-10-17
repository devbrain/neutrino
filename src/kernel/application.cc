//
// Created by igor on 07/10/2021.
//

#include <neutrino/kernel/application.hh>
#include <neutrino/utils/exception.hh>

#include "systems_manager.hh"
#include "main_window.hh"

namespace neutrino::kernel {

  static application* s_instance = nullptr;

  struct application::impl {
    impl() : m_paused(false) {};

    explicit impl(hal::window_flags_t flags)
    : m_main_window(flags) {}

    void init() {
      init_video();
    }

    void init_video() {
      m_main_window.set_up (get_systems_manager()->get_video_system());
    }
    main_window m_main_window;
    bool m_paused;
  };


  application::~application() {
    release_systems_manager();
  }

  void application::on_terminating () {
    get_systems_manager()->on_terminating();
  }

  void application::on_low_memory () {
    get_systems_manager()->on_low_memory();
  }

  void application::on_will_enter_background () {
    get_systems_manager()->on_will_enter_background();
  }

  void application::on_in_background () {
    get_systems_manager()->on_in_background();
  }

  void application::on_in_foreground () {
    get_systems_manager()->on_in_foreground();
  }


  void application::clear () {
    if (!m_pimpl->m_paused) {
      hal::application::clear ();
    }
  }
  void application::update (std::chrono::milliseconds ms) {
    if (!m_pimpl->m_paused) {
      get_systems_manager ()->update (ms);
    }
  }

  void application::render () {
    get_systems_manager()->present();
    hal::application::render();
  }

  void application::add_system(std::unique_ptr<base_input_system> input_sys) {
    get_systems_manager()->add (std::move(input_sys));
  }

  void application::add_system(std::unique_ptr<video_system> video_sys) {
    get_systems_manager()->add (std::move(video_sys));
  }

  [[maybe_unused]] void application::add_system(std::unique_ptr<system> sys) {
    get_systems_manager()->add (std::move(sys));
  }

  void application::show (int w, int h) {
    m_pimpl = spimpl::make_unique_impl<impl>();
    m_pimpl->m_main_window.open (w, h);
    m_pimpl->init();
  }

  void application::show (int w, int h, hal::window_flags_t flags) {
    m_pimpl = spimpl::make_unique_impl<impl>(flags);
    m_pimpl->m_main_window.open (w, h);
    m_pimpl->init();
  }

  void application::show (int w, int h, int x, int y, hal::window_flags_t flags) {
    m_pimpl = spimpl::make_unique_impl<impl>(flags);
    m_pimpl->m_main_window.open (w, h, x, y);
    m_pimpl->init();
  }

  void application::toggle_full_screen () {
    m_pimpl->m_main_window.toggle_fullscreen();
  }

  void application::set_title (const std::string& title) {
    m_pimpl->m_main_window.title (title);
  }

  void application::post_init () {
    if (!s_instance) {
      s_instance = this;
    } else {
      RAISE_EX("Application is already constructed");
    }
  }

  void application::pause(bool v) {
    m_pimpl->m_paused = v;
    get_systems_manager()->on_paused (v);
  }

  bool application::paused() const noexcept {
    return m_pimpl->m_paused;
  }
  application* get_application() {
    return s_instance;
  }

}