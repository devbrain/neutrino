//
// Created by igor on 04/03/2026.
//

#include <exception>
#include <limits>
#include <sdlpp/events/events.hh>
#include <failsafe/enforce.hh>
#include <failsafe/logger.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>
#include "scenes_manager.hh"


namespace neutrino {
    static constexpr auto INVALID_EVENT = std::numeric_limits <uint32_t>::max();
    static uint32_t s_event_type = INVALID_EVENT;
    static constexpr int PUSH_SCENE = 1;
    static constexpr int POP_SCENE = 2;
    static constexpr int REPLACE_SCENE = 3;

    static SDL_Event create_custom_event(int code, base_scene* bs = nullptr) {
        if (s_event_type == INVALID_EVENT) {
            auto ids = sdlpp::event_registry::register_events(1);
            ENFORCE(ids);
            s_event_type = *ids;
        }
        SDL_Event custom{};
        custom.type = s_event_type;
        custom.user.code = code;
        custom.user.data1 = bs;
        custom.user.data2 = nullptr;
        return custom;
    }

    scenes_manager::scenes_manager() {
        static bool initialized = false;
        ENFORCE(!initialized);
        initialized = true;
    }

    scenes_manager::~scenes_manager() = default;

    void scenes_manager::push_scene(std::unique_ptr <base_scene>&& scene) {
        auto v = std::move(scene);
        const auto ev = create_custom_event(PUSH_SCENE, v.release());
        ENFORCE (sdlpp::event_queue::push(sdlpp::event(ev)));
    }

    void scenes_manager::pop_scene() {
        auto ev = create_custom_event(POP_SCENE);
        ENFORCE(sdlpp::event_queue::push(sdlpp::event(ev)));
    }

    void scenes_manager::replace_scene(std::unique_ptr <base_scene>&& scene) {
        auto v = std::move(scene);
        auto ev = create_custom_event(REPLACE_SCENE, v.release());
        ENFORCE(sdlpp::event_queue::push(sdlpp::event(ev)));
    }

    void scenes_manager::push_scene_sync(std::unique_ptr <base_scene>&& scene) {
        apply_push(std::move(scene));
    }

    void scenes_manager::finish() {
        // Drain pending scene-transition events so released pointers are reclaimed
        // Note: using SDL_PeepEvents directly — no sdlpp wrapper available
        SDL_Event ev;
        while (SDL_PeepEvents(&ev, 1, SDL_GETEVENT, s_event_type, s_event_type) > 0) {
            if (ev.user.data1) {
                delete static_cast <base_scene*>(ev.user.data1);
            }
        }
        // Orderly teardown of the scene stack (top to bottom)
        while (!m_stack.empty()) {
            m_stack.back()->on_exit();
            m_stack.pop_back();
        }
    }

    void scenes_manager::apply_push(std::unique_ptr <base_scene>&& scene) {
        // Non-opaque scenes (e.g. busy_scene) are overlays: the scene below
        // keeps running, just with a visual/input overlay on top. It must
        // NOT receive on_pause, otherwise any refresh logic wired to
        // on_resume would fire every time the overlay is dismissed —
        // spawning duplicate background syncs during in-flight async work.
        const bool overlay = !scene->is_opaque();
        if (!overlay && !m_stack.empty()) {
            m_stack.back()->on_pause();
        }
        m_stack.emplace_back(std::move(scene));

        // Isolate the new scene's on_enter. If initialization throws (missing
        // asset, bad data, etc.) we roll back cleanly: give the partially-
        // entered scene a chance to clean up, drop it from the stack, resume
        // the scene below, and surface the error via the global queue. The
        // app stays responsive instead of letting the exception escape to
        // SDL, which would turn it into SDL_APP_FAILURE and terminate.
        try {
            m_stack.back()->on_enter();
        } catch (const std::exception& ex) {
            LOG_ERROR("Scene on_enter threw:", ex.what());
            try { m_stack.back()->on_exit(); } catch (...) {}
            m_stack.pop_back();
            if (!overlay && !m_stack.empty()) {
                try { m_stack.back()->on_resume(); } catch (...) {}
            }
            // TODO
      //      services::report_error(ex.what());
        } catch (...) {
            LOG_ERROR("Scene on_enter threw an unknown exception");
            try { m_stack.back()->on_exit(); } catch (...) {}
            m_stack.pop_back();
            if (!overlay && !m_stack.empty()) {
                try { m_stack.back()->on_resume(); } catch (...) {}
            }
            // TODO

            // services::report_error(
            //     "An unexpected error occurred while opening that screen.");
        }
    }

    void scenes_manager::apply_pop() {
        ENFORCE(!m_stack.empty());
        const bool overlay = !m_stack.back()->is_opaque();
        m_stack.back()->on_exit();
        m_stack.pop_back();
        // Mirror apply_push: popping an overlay is not a real mode change,
        // so the scene below doesn't get on_resume.
        if (!overlay && !m_stack.empty()) {
            m_stack.back()->on_resume();
        }
    }

    void scenes_manager::apply_replace(std::unique_ptr <base_scene>&& scene) {
        ENFORCE(!m_stack.empty());
        m_stack.back()->on_exit();
        m_stack.back() = std::move(scene);
        m_stack.back()->on_enter();
    }

    void scenes_manager::update_physics(frame_duration delta_t) {
        if (m_stack.empty()) return;

        // Propagate physics to every scene from the first opaque one upward,
        // mirroring render(). This keeps scenes beneath a non-opaque overlay
        // (e.g. busy_scene) alive: their async_task::poll() keeps running,
        // so completion callbacks fire while a spinner is shown on top.
        size_t index = 0;
        for (size_t i = m_stack.size(); i-- > 0;) {
            if (m_stack[i]->is_opaque()) {
                index = i;
                break;
            }
        }
        for (size_t i = index; i < m_stack.size(); i++) {
            m_stack[i]->update_physics(delta_t);
        }
    }

    void scenes_manager::render(frame_duration time_since_last_frame) {
        if (m_stack.empty()) return;

        size_t index = 0;
        for (size_t i = m_stack.size(); i-- > 0;) {
            if (m_stack[i]->is_opaque()) {
                index = i;
                break;
            }
        }

        for (size_t i = index; i < m_stack.size(); i++) {
            m_stack[i]->render(time_since_last_frame);
        }
    }

    void scenes_manager::handle_action(const sdlpp::event& ev) {
        if (sdlpp::event_registry::is_custom(ev.type())) {
            auto* ue = ev.as <sdlpp::user_event>();
            auto type = static_cast <uint32_t>(ue->type);
            if (type == s_event_type && type != INVALID_EVENT) {
                if (ue->code == PUSH_SCENE) {
                    ENFORCE(ue->data1);
                    apply_push(std::unique_ptr <base_scene>(static_cast <base_scene*>(ue->data1)));
                } else if (ue->code == POP_SCENE) {
                    apply_pop();
                } else if (ue->code == REPLACE_SCENE) {
                    ENFORCE(ue->data1);
                    apply_replace(std::unique_ptr <base_scene>(static_cast <base_scene*>(ue->data1)));
                } else {
                    THROW_DEFAULT("Should not be here");
                }
                return;
            }
        }
        if (!m_stack.empty()) {
            m_stack.back()->handle_action(ev);
        }
    }

    bool scenes_manager::empty() const {
        return m_stack.empty();
    }

    void push_scene(std::unique_ptr <base_scene> scene) {
        scenes_manager::push_scene(std::move(scene));
    }

    void pop_scene() {
        scenes_manager::pop_scene();
    }

    void replace_scene(std::unique_ptr <base_scene> scene) {
        scenes_manager::replace_scene(std::move(scene));
    }
}
