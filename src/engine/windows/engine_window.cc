//
// Created by igor on 13/06/2021.
//

#include <neutrino/engine/windows/engine_window.hh>
#include <engine/events/events_mapper.hh>

namespace neutrino::engine {
    // --------------------------------------------------------------------------------------------
    engine_window::engine_window(std::shared_ptr<basic_renderer> renderer, window_flags_t flags)
    : basic_window(renderer->window_kind(), flags),
      m_renderer(renderer) {

    }
    // --------------------------------------------------------------------------------------------
    engine_window::engine_window(std::shared_ptr<basic_renderer> renderer)
    : basic_window(renderer->window_kind()),
      m_renderer(renderer) {

    }
    // --------------------------------------------------------------------------------------------
    void engine_window::on_window_close()
    {
        this->notify(events::before_window_closed{});
    }
    // --------------------------------------------------------------------------------------------
    void engine_window::on_window_resized(int w, int h) {
        this->notify(events::window_resized{w,h});
    }
    // --------------------------------------------------------------------------------------------
    void engine_window::on_input_focus_changed(bool keyboard_focus, bool mouse_focus) {
        this->notify(events::input_focus_changed{keyboard_focus, mouse_focus});
    }
    // --------------------------------------------------------------------------------------------
    void engine_window::on_visibility_change(bool is_visible) {
        this->notify(events::visibility_changed{is_visible});
    }
    // --------------------------------------------------------------------------------------------
    void engine_window::on_keyboard_input(const sdl::events::keyboard& ev) {
        this->notify(events::map_event(ev));
    }
    // --------------------------------------------------------------------------------------------
    void engine_window::on_mouse_button(const sdl::events::mouse_button& ev) {
        this->notify(events::map_event(ev));
    }
    // --------------------------------------------------------------------------------------------
    void engine_window::on_mouse_motion(const sdl::events::mouse_motion& ev) {
        this->notify(events::map_event(ev));
    }
    // --------------------------------------------------------------------------------------------
    void engine_window::on_mouse_wheel(const sdl::events::mouse_wheel& ev) {
        this->notify(events::map_event(ev));
    }
    // --------------------------------------------------------------------------------------------
    void engine_window::on_touch_button(const sdl::events::touch_device_button& ev) {
        this->notify(events::map_event(ev));
    }
    // --------------------------------------------------------------------------------------------
    void engine_window::on_touch_motion(const sdl::events::touch_device_motion& ev) {
        this->notify(events::map_event(ev));
    }
    // --------------------------------------------------------------------------------------------
    void engine_window::on_touch_wheel(const sdl::events::touch_device_wheel& ev) {
        this->notify(events::map_event(ev));
    }
    // --------------------------------------------------------------------------------------------
    void engine_window::clear() {
        m_renderer->clear();
    }
    // --------------------------------------------------------------------------------------------
    void engine_window::after_window_opened([[maybe_unused]] uint32_t window_id) {
        m_renderer->open(*this);
    }
    // --------------------------------------------------------------------------------------------
    void engine_window::present() {
        m_renderer->present();
    }

}