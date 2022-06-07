//
// Created by igor on 02/06/2022.
//

#include <stack>

#include <neutrino/kernel/scene/scene_manager.hh>
#include "neutrino/kernel/scene/basic_scene.hh"
#include <neutrino/kernel/application.hh>
#include <neutrino/utils/exception.hh>


namespace neutrino::kernel {
  struct scene_manager::impl {

    impl(context& ctx, neutrino::application* app)
    : m_context(ctx), m_app(app), m_events(app->events()) {}

    context& m_context;
    neutrino::application* m_app;
    events_holder& m_events;
    std::stack<std::unique_ptr<basic_scene>> m_stack;
  };

  scene_manager::scene_manager(context& ctx, neutrino::application* app, [[maybe_unused]] pass_token tk)
  : m_pimpl(spimpl::make_unique_impl<impl>(ctx, app)) {

  }

  scene_manager::~scene_manager() = default;

  void scene_manager::push(std::unique_ptr<basic_scene> sc) {
    m_pimpl->m_stack.push (std::move(sc));
    m_pimpl->m_stack.top()->m_manager = this;
  }

  void scene_manager::pop() {
    ENFORCE(!m_pimpl->m_stack.empty());
    m_pimpl->m_stack.pop();
  }

  void scene_manager::replace(std::unique_ptr<basic_scene> sc) {
    if (!m_pimpl->m_stack.empty()) {
      m_pimpl->m_stack.top() = std::move(sc);
    } else {
      m_pimpl->m_stack.push (std::move (sc));
    }
    m_pimpl->m_stack.top()->m_manager = this;
  }

  void scene_manager::update_logic (std::chrono::milliseconds ms) {
    if (m_pimpl->m_stack.empty()) {
      m_pimpl->m_app->close();
    }
    auto& sc = m_pimpl->m_stack.top();
    sc->handle_input (m_pimpl->m_events, m_pimpl->m_context);
    sc->update_logic (ms, m_pimpl->m_context);
  }

  void scene_manager::draw_frame () {
    if (m_pimpl->m_stack.empty()) {
      m_pimpl->m_app->close();
    }
    m_pimpl->m_stack.top()->present(m_pimpl->m_context);
  }
}
