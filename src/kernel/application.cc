//
// Created by igor on 07/10/2021.
//

#include <neutrino/kernel/application.hh>
#include <neutrino/kernel/system/context.hh>
#include <neutrino/kernel/scene/scene_manager.hh>

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
          m_pointer_mapper (m_events),
          m_ctx(nullptr) {
    }

    ~impl() {
      delete m_ctx;
    }

    std::unique_ptr<main_window> window;
    bool paused;
    hal::renderer* renderer;

    events_holder m_events;
    kbd_mapper m_kbd_mapper;
    pointer_mapper m_pointer_mapper;
    ecs::registry m_registry;

    kernel::context* m_ctx;

    std::unique_ptr<kernel::scene_manager> m_scene_manager;
  };

  application::application () {
    m_pimpl = std::make_unique<application::impl>();
  }

  application::~application () = default;

  void application::execute () {
    auto app_descr = this->describe();
    auto mw_desc = app_descr.get_main_window_description();
    m_pimpl->window = std::make_unique<main_window>(mw_desc, this);
    m_pimpl->window->show();
    m_pimpl->renderer = m_pimpl->window->renderer();

    m_pimpl->m_ctx = new kernel::context(kernel::audio_system{}, kernel::video_system(*m_pimpl->renderer,
                                                                                      math::dimension2di_t (mw_desc.width(), mw_desc.height())));

    m_pimpl->m_scene_manager = std::make_unique<kernel::scene_manager>(*m_pimpl->m_ctx, this, kernel::scene_manager::pass_token{});

    this->init (*m_pimpl->m_ctx);
    this->run(static_cast<int>(app_descr.fps()));
    this->on_exit();
  }

  void application::on_exit () {
  }

  void application::on_terminating () {
    hal::application::on_terminating ();
  }

  void application::on_low_memory () {
    hal::application::on_low_memory ();
  }

  void application::on_will_enter_background () {
    hal::application::on_will_enter_background ();
  }

  void application::on_in_background () {
    hal::application::on_in_background ();
  }

  void application::on_in_foreground () {
    hal::application::on_in_foreground ();
  }

  void application::on_window_resized ([[maybe_unused]] unsigned int new_w, [[maybe_unused]] unsigned int new_h) {
  }

  void application::on_paused () {
  }

  void application::on_resumed () {
  }

  void application::close () {
    if (m_pimpl->window) {
      m_pimpl->window->close();
    }
  }

  void application::toggle_fullscreen () {
    if (m_pimpl->window) {
      m_pimpl->window->toggle_fullscreen();
    }
  }

  void application::update (std::chrono::milliseconds ms) {
    if (!m_pimpl->paused) {
      update_logic (ms);
    }
    m_pimpl->m_events.reset();
  }

  void application::on_keyboard_input (const hal::events::keyboard& ev) {
    m_pimpl->m_kbd_mapper.handle_event (ev);
  }

  void application::on_pointer_input (const hal::events::pointer& ev) {
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

  kernel::scene_manager& application::manager() {
    return *m_pimpl->m_scene_manager;
  }

  pointer_config_base& application::mouse_config() {
    return m_pimpl->m_pointer_mapper;
  }

  ecs::id_t application::create_entity() const {
    return m_pimpl->m_registry.create_id();
  }

  ecs::registry& application::registry() {
    return m_pimpl->m_registry;
  }

  const ecs::registry& application::registry() const {
    return m_pimpl->m_registry;
  }

  void application::update_logic(std::chrono::milliseconds ms) {
    m_pimpl->m_scene_manager->update_logic (ms);
  }

  void application::draw_frame() {
    m_pimpl->m_scene_manager->draw_frame();
  }

}