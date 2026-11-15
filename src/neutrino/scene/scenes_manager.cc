//
// Created by igor on 04/03/2026.
//

#include <exception>
#include <limits>
#include <mutex>
#include <unordered_map>
#include <sdlpp/events/events.hh>
#include <failsafe/enforce.hh>
#include <failsafe/logger.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>
#include <neutrino/video/globals.hh>
#include "scenes_manager.hh"

namespace neutrino {
    static constexpr auto INVALID_EVENT = std::numeric_limits <uint32_t>::max();
    static uint32_t s_event_type = INVALID_EVENT;
    static bool s_scenes_manager_initialized = false;
    static constexpr int PUSH_SCENE = 1;
    static constexpr int POP_SCENE = 2;
    static constexpr int REPLACE_SCENE = 3;
    static constexpr int PIN_SCENE = 4;
    static constexpr int UNPIN_SCENE = 5;
    // Pending transitions outlive a consumed event when an application replaces
    // scene dispatch. Keep ownership here until dispatch or finish(), not in SDL.
    static std::mutex s_pending_mutex;
    static std::unordered_map <base_scene*, std::unique_ptr <base_scene>> s_pending_scenes;

    static std::unique_ptr <base_scene> take_pending_scene(void* id) {
        const std::lock_guard lock(s_pending_mutex);
        auto node = s_pending_scenes.extract(static_cast <base_scene*>(id));
        return node.empty() ? nullptr : std::move(node.mapped());
    }

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

    static void queue_scene(int code, std::unique_ptr <base_scene> scene) {
        ENFORCE(scene != nullptr);
        const auto ev = create_custom_event(code, scene.get());
        {
            const std::lock_guard lock(s_pending_mutex);
            s_pending_scenes.emplace(scene.get(), std::move(scene));
        }
        try {
            ENFORCE(sdlpp::event_queue::push(sdlpp::event(ev)));
        } catch (...) {
            auto discarded = take_pending_scene(ev.user.data1);
            throw;
        }
    }

    scenes_manager::scenes_manager() {
        ENFORCE(!s_scenes_manager_initialized);
        s_scenes_manager_initialized = true;
    }

    scenes_manager::~scenes_manager() {
        s_scenes_manager_initialized = false;
    }

    void scenes_manager::push_scene(std::unique_ptr <base_scene>&& scene) {
        queue_scene(PUSH_SCENE, std::move(scene));
    }

    void scenes_manager::pop_scene() {
        auto ev = create_custom_event(POP_SCENE);
        ENFORCE(sdlpp::event_queue::push(sdlpp::event(ev)));
    }

    void scenes_manager::pin_scene(std::unique_ptr <base_scene>&& scene) {
        queue_scene(PIN_SCENE, std::move(scene));
    }

    void scenes_manager::unpin_scene() {
        auto ev = create_custom_event(UNPIN_SCENE);
        ENFORCE(sdlpp::event_queue::push(sdlpp::event(ev)));
    }

    void scenes_manager::replace_scene(std::unique_ptr <base_scene>&& scene) {
        queue_scene(REPLACE_SCENE, std::move(scene));
    }

    void scenes_manager::push_scene_sync(std::unique_ptr <base_scene>&& scene) {
        apply_push(std::move(scene));
    }

    void scenes_manager::finish() {
        // Remove queued transition notifications, then reclaim pending scenes,
        // including requests whose events were ignored by an application override.
        sdlpp::event_queue::flush(static_cast <sdlpp::event_type>(s_event_type));
        {
            decltype(s_pending_scenes) discarded;
            {
                const std::lock_guard lock(s_pending_mutex);
                discarded.swap(s_pending_scenes);
            }
            // Destroy outside the lock: scene destructors can call engine services.
        }
        // Orderly teardown of the scene stack (top to bottom).
        //
        // Each on_exit is guarded INDIVIDUALLY and the scene is popped either way. A single throw
        // used to escape this loop with that scene and everything under it still on the stack --
        // and shutdown continues regardless, so the application's own teardown would then release
        // assets those surviving scenes still reference, leaving them to be destroyed later
        // against dead services. One scene failing to exit cleanly must not strand the rest.
        while (!m_stack.empty()) {
            try {
                m_stack.back()->on_exit();
            } catch (const std::exception& e) {
                LOG_ERROR("Scene on_exit threw during shutdown:", e.what());
            } catch (...) {
                LOG_ERROR("Scene on_exit threw an unknown exception during shutdown");
            }
            m_stack.pop_back(); // pop regardless: the stack must be empty when finish() returns
        }
        m_pinned = false;
    }

    std::size_t scenes_manager::active_count() const noexcept {
        return m_stack.size() - (m_pinned ? 1 : 0);
    }

    base_scene* scenes_manager::active_top() const noexcept {
        const std::size_t n = active_count();
        return n == 0 ? nullptr : m_stack[n - 1].get();
    }

    bool scenes_manager::enter_or_roll_back(base_scene* scene) {
        // Isolate the new scene's on_enter. If initialization throws (missing
        // asset, bad data, etc.) we roll back cleanly: give the partially-
        // entered scene a chance to clean up and drop it from the stack. The
        // app stays responsive instead of letting the exception escape to
        // SDL, which would turn it into SDL_APP_FAILURE and terminate.
        try {
            scene->on_enter();
            return true;
        } catch (const std::exception& ex) {
            LOG_ERROR("Scene on_enter threw:", ex.what());
        } catch (...) {
            LOG_ERROR("Scene on_enter threw an unknown exception");
        }
        try { scene->on_exit(); } catch (...) {}
        for (auto i = m_stack.begin(); i != m_stack.end(); ++i) {
            if (i->get() == scene) {
                m_stack.erase(i);
                break;
            }
        }
        return false;
    }

    void scenes_manager::apply_push(std::unique_ptr <base_scene>&& scene) {
        // on_pause / on_resume follow pauses_below: they tell the scene below that its
        // fixed_update stops and starts again. A scene without pauses_below (e.g. busy_scene)
        // leaves the scene below running, so that scene must NOT receive on_pause, otherwise
        // any refresh logic wired to on_resume would fire every time the overlay is
        // dismissed -- spawning duplicate background syncs during in-flight async work.
        // hides_below is irrelevant here: a hidden scene that still updates is not paused.
        // The new scene goes on top of the stack, below the pinned scene if there is one.
        const bool pauses = has(scene->flags(), scene_flags::pauses_below);
        if (base_scene* below = active_top(); pauses && below) {
            below->on_pause();
        }
        const auto at = m_stack.begin() + static_cast <std::ptrdiff_t>(active_count());
        base_scene* const entered = m_stack.insert(at, std::move(scene))->get();
        if (!enter_or_roll_back(entered) && pauses) {
            if (base_scene* below = active_top()) {
                try { below->on_resume(); } catch (...) {}
            }
        }
        // Whatever ended up on top (the pushed scene, or the resumed scene below
        // after a rolled-back on_enter) becomes active now: size it once here so
        // it never has to poll the render size.
        notify_top_resized();
    }

    void scenes_manager::apply_pop() {
        ENFORCE(active_count() > 0, "pop_scene: the stack has no scene to pop");
        const std::size_t top = active_count() - 1;
        const bool pauses = has(m_stack[top]->flags(), scene_flags::pauses_below);
        m_stack[top]->on_exit();
        m_stack.erase(m_stack.begin() + static_cast <std::ptrdiff_t>(top));
        // Mirror apply_push: only a scene that paused the one below resumes it.
        if (base_scene* below = active_top(); pauses && below) {
            below->on_resume();
        }
        notify_top_resized();
    }

    void scenes_manager::apply_replace(std::unique_ptr <base_scene>&& scene) {
        ENFORCE(active_count() > 0, "replace_scene: the stack has no scene to replace");
        auto& slot = m_stack[active_count() - 1];
        slot->on_exit();
        slot = std::move(scene);
        slot->on_enter();
        notify_top_resized();
    }

    void scenes_manager::apply_pin(std::unique_ptr <base_scene>&& scene) {
        apply_unpin(); // pinning replaces the pinned scene, if any
        const bool pauses = has(scene->flags(), scene_flags::pauses_below);
        if (base_scene* below = active_top(); pauses && below) {
            below->on_pause();
        }
        m_stack.emplace_back(std::move(scene));
        m_pinned = true;
        base_scene* const pinned = m_stack.back().get();
        if (enter_or_roll_back(pinned)) {
            pinned->on_resize(render_size());
            return;
        }
        m_pinned = false;
        if (base_scene* below = active_top(); pauses && below) {
            try { below->on_resume(); } catch (...) {}
        }
    }

    void scenes_manager::apply_unpin() {
        if (!m_pinned) {
            return;
        }
        const bool pauses = has(m_stack.back()->flags(), scene_flags::pauses_below);
        m_stack.back()->on_exit();
        m_stack.pop_back();
        m_pinned = false;
        if (base_scene* below = active_top(); pauses && below) {
            below->on_resume();
        }
    }

    void scenes_manager::notify_top_resized() {
        if (base_scene* top = active_top()) {
            top->on_resize(render_size());
        }
    }

    void scenes_manager::notify_resize(dim size) {
        if (base_scene* top = active_top()) {
            top->on_resize(size);
        }
        if (m_pinned) {
            m_stack.back()->on_resize(size);
        }
    }

    void scenes_manager::fixed_update(sim_duration dt, const input_snapshot& in) {
        if (m_stack.empty()) return;

        // Propagate the fixed-step update to every scene from the topmost pauses_below one
        // upward. This is independent of render(), which walks to hides_below instead. Scenes
        // beneath an overlay without pauses_below (e.g. busy_scene) stay alive: their
        // async_task::poll() keeps running, so completion callbacks fire while a spinner is
        // shown on top.
        size_t index = 0;
        for (size_t i = m_stack.size(); i-- > 0;) {
            const auto flags = m_stack[i]->flags();
            if (has(flags, scene_flags::pauses_below)) {
                index = i;
                break;
            }
        }
        // Live input runs from the top scene down through scenes with passes_input and stops at
        // the first scene without it -- or at the topmost pauses_below scene, since a paused
        // scene gets no input. Raw events follow the same chain (handle_action). Every other
        // covered scene keeps updating (that is the point of running it under an overlay) but
        // must not act on the pointer or buttons: handing it the real snapshot would let a
        // background scene react to clicks *through* a modal or busy overlay. It gets a neutral
        // snapshot -- nothing held, nothing pressed, pointer off-screen.
        size_t floor = m_stack.size() - 1;
        while (floor > index && has(m_stack[floor]->flags(), scene_flags::passes_input)) {
            --floor;
        }

        static constexpr input_snapshot no_input{};
        for (size_t i = index; i < m_stack.size(); i++) {
            m_stack[i]->fixed_update(dt, i >= floor ? in : no_input);
        }
    }

    void scenes_manager::render() {
        if (m_stack.empty()) return;

        // Draw every scene from the topmost hides_below one upward, bottom to top.
        size_t index = 0;
        for (size_t i = m_stack.size(); i-- > 0;) {
            if (has(m_stack[i]->flags(), scene_flags::hides_below)) {
                index = i;
                break;
            }
        }

        for (size_t i = index; i < m_stack.size(); i++) {
            m_stack[i]->render();
        }
    }

    void scenes_manager::handle_action(const sdlpp::event& ev) {
        if (sdlpp::event_registry::is_custom(ev.type())) {
            auto* ue = ev.as <sdlpp::user_event>();
            auto type = static_cast <uint32_t>(ue->type);
            if (type == s_event_type && type != INVALID_EVENT) {
                if (ue->code == PUSH_SCENE) {
                    auto scene = take_pending_scene(ue->data1);
                    ENFORCE(scene != nullptr);
                    apply_push(std::move(scene));
                } else if (ue->code == POP_SCENE) {
                    apply_pop();
                } else if (ue->code == REPLACE_SCENE) {
                    auto scene = take_pending_scene(ue->data1);
                    ENFORCE(scene != nullptr);
                    apply_replace(std::move(scene));
                } else if (ue->code == PIN_SCENE) {
                    auto scene = take_pending_scene(ue->data1);
                    ENFORCE(scene != nullptr);
                    apply_pin(std::move(scene));
                } else if (ue->code == UNPIN_SCENE) {
                    apply_unpin();
                } else {
                    THROW_DEFAULT("Should not be here");
                }
                return;
            }
        }
        if (!m_stack.empty()) {
            // Events follow the same chain as the live input in fixed_update: from the top down
            // through passes_input scenes, never past a pauses_below one. The flags are read
            // before delivering, so a handle_action that changes them routes the next event.
            for (size_t i = m_stack.size(); i-- > 0;) {
                const auto f = m_stack[i]->flags();
                m_stack[i]->handle_action(ev);
                if (!has(f, scene_flags::passes_input) || has(f, scene_flags::pauses_below)) {
                    break;
                }
            }
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

    void pin_scene(std::unique_ptr <base_scene> scene) {
        scenes_manager::pin_scene(std::move(scene));
    }

    void unpin_scene() {
        scenes_manager::unpin_scene();
    }
}
