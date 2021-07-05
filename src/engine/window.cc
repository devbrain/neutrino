//
// Created by igor on 02/07/2021.
//

#include <neutrino/engine/scene_manager.hh>
#include "window.hh"

namespace neutrino::engine {
    window::window(const std::shared_ptr<abstract_graphics_context>& graphics_context, hal::window_flags_t flags)
    : hal::window(graphics_context->window_kind(), flags),
      m_graphics_context(graphics_context)
    {

    }
    // -----------------------------------------------------------------------------------------------------------
    window::window(const std::shared_ptr<abstract_graphics_context>& graphics_context)
    : hal::window(graphics_context->window_kind()),
     m_graphics_context(graphics_context)
    {

    }
    // -----------------------------------------------------------------------------------------------------------
    void window::clear() {
        m_graphics_context->clear();
    }
    // -----------------------------------------------------------------------------------------------------------
    void window::present() {
        m_graphics_context->present();
    }
    // -----------------------------------------------------------------------------------------------------------
    void window::show() {
        auto [w,h] = m_graphics_context->dimensions();
        this->open(w, h);
        m_graphics_context->attach(*this);
    }
    // -----------------------------------------------------------------------------------------------------------
    void window::show(int x, int y) {
        auto [w,h] = m_graphics_context->dimensions();
        this->open(w, h, x, y);
        m_graphics_context->attach(*this);
    }
    // -----------------------------------------------------------------------------------------------------------
    void window::on_event(const events::full_screen&) {
        this->toggle_fullscreen();
    }
    // -----------------------------------------------------------------------------------------------------------
    void window::before_window_destroy() {
        scene_manager::instance().on_before_destroy();
    }
    // -----------------------------------------------------------------------------------------------------------
    void window::on_input_focus_changed(bool keyboard_focus, bool mouse_focus) {
        scene_manager::instance().on_input_focus_changed(keyboard_focus, mouse_focus);
    }
    // -----------------------------------------------------------------------------------------------------------
    void window::on_visibility_change(bool is_visible) {
        scene_manager::instance().on_visibility_change(is_visible);
    }
    // -----------------------------------------------------------------------------------------------------------
    void window::on_keyboard_input(const events::keyboard& ev) {
        scene_manager::instance().on_keyboard_input(ev);
    }
    // -----------------------------------------------------------------------------------------------------------
    void window::on_pointer_input(const events::pointer& ev) {
        scene_manager::instance().on_pointer_input(ev);
    }
    // -----------------------------------------------------------------------------------------------------------
    void window::on_window_invalidate() {
        m_graphics_context->invalidate(*this);
    }
}
