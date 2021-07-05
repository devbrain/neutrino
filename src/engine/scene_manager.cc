//
// Created by igor on 03/07/2021.
//

#include <neutrino/engine/scene_manager.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::engine::detail {
    scene_manager::scene_manager()
    : m_current_scene(nullptr),
      m_first_time(true)
    {

    }
    // ----------------------------------------------------------------------------------------
    void scene_manager::add(scene* sc) {
        auto itr = m_scenes.find(sc->id());
        if (itr != m_scenes.end()) {
            RAISE_EX("Scene ", sc->id(), " was already registered");
        }
        m_scenes[sc->id()] = sc;
    }
    // ----------------------------------------------------------------------------------------
    void scene_manager::activate(int scene_id) {
        auto itr = m_scenes.find(scene_id);
        if (itr == m_scenes.end()) {
            RAISE_EX("Unknow scene ", scene_id);
        }
        if (m_current_scene) {
            m_current_scene->on_exit();
        }
        m_current_scene = itr->second;
        if (!m_first_time)
        {
            m_current_scene->on_enter();
        }
    }
    // ----------------------------------------------------------------------------------------
    void scene_manager::update(std::chrono::milliseconds ms) {
        if (m_current_scene)
        {
            if (m_first_time) {
                m_current_scene->on_enter();
                m_first_time = false;
            }
            m_current_scene->update(ms);
        }
    }
    // ----------------------------------------------------------------------------------------
    void scene_manager::on_before_destroy() {

    }
    // ----------------------------------------------------------------------------------------
    void scene_manager::on_input_focus_changed(bool keyboard_focus, bool mouse_focus) {
        if (m_current_scene) {
            m_current_scene->on_input_focus_changed(keyboard_focus, mouse_focus);
        }
    }
    // ----------------------------------------------------------------------------------------
    void scene_manager::on_visibility_change(bool is_visible) {
        if (m_current_scene) {
            m_current_scene->on_visibility_change(is_visible);
        }
    }
    // ----------------------------------------------------------------------------------------
    void scene_manager::on_keyboard_input(const events::keyboard& ev) {
        if (m_current_scene) {
            m_current_scene->on_keyboard_input(ev);
        }
    }
    // ----------------------------------------------------------------------------------------
    void scene_manager::on_pointer_input(const events::pointer& ev) {
        if (m_current_scene) {
            m_current_scene->on_pointer_input(ev);
        }
    }
}