//
// Created by igor on 02/07/2021.
//

#ifndef NEUTRINO_WINDOW_HH
#define NEUTRINO_WINDOW_HH

#include <memory>
#include <neutrino/hal/video/window.hh>
#include <neutrino/utils/observer.hh>
#include <neutrino/engine/events.hh>
#include <neutrino/engine/graphics/abstract_context.hh>

namespace neutrino::engine
{
    class window : public hal::window, public utils::observer<events::full_screen>
    {
    public:
        window(const std::shared_ptr<abstract_graphics_context>& graphics_context, hal::window_flags_t flags);
        window(const std::shared_ptr<abstract_graphics_context>& graphics_context);

        void show();
        void show(int x, int y);

        void on_event(const events::full_screen&) override;
    private:
        void clear() override;
        void present() override;
        void before_window_destroy() override;
        void on_input_focus_changed(bool keyboard_focus, bool mouse_focus) override;
        void on_visibility_change(bool is_visible) override;
        void on_keyboard_input(const events::keyboard& ev) override;
        void on_pointer_input(const events::pointer& ev) override;
        void on_window_invalidate() override;
    private:
        std::shared_ptr<abstract_graphics_context> m_graphics_context;
    };
}


#endif //NEUTRINO_WINDOW_HH
