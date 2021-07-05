//
// Created by igor on 03/07/2021.
//

#ifndef NEUTRINO_MAIN_SCENE_HH
#define NEUTRINO_MAIN_SCENE_HH

#include <neutrino/engine/scene.hh>
#include <neutrino/demoscene/demoscene.hh>
#include <neutrino/utils/observer.hh>

namespace neutrino::demoscene
{
    class main_scene : public engine::scene, public utils::observer<engine::events::current_fps>
    {
    public:
        main_scene(demoscene::scene* owner);
        void on_event(const engine::events::current_fps& e) override;
    private:
        void on_enter() override;
        void on_exit() override;

        void update(std::chrono::milliseconds ms) override;
        void on_input_focus_changed(bool keyboard_focus, bool mouse_focus) override;
        void on_visibility_change(bool is_visible) override;
        void on_keyboard_input(const engine::events::keyboard& ev) override;
        void on_pointer_input(const engine::events::pointer& ev) override;
    private:
        demoscene::scene* m_owner;
        palette_t* m_pal;
        surface_t* m_surface;
        int m_width;
        int m_height;
        bool m_show_fps;
    };
}


#endif //NEUTRINO_MAIN_SCENE_HH
