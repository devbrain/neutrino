//
// Created by igor on 04/03/2026.
//

#include <exception>
#include <limits>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>
#include <sdlpp/events/events.hh>
#include <sdlpp/video/window.hh>
#include <failsafe/enforce.hh>
#include <failsafe/logger.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>
#include <neutrino/video/globals.hh>
#include "scenes_manager.hh"
#include "scene_context_access.hh"
#include "scene_frame.hh"
#include "services/service_locator.hh"
#include "video/canvas.hh"
#include "video/composite_draw.hh"
#include "video/layer_stack.hh"
#include "video/painter_context.hh"
#include "video/painter_state_stack.hh"
#include "events/event_types.hh"
#include "input/action_tables.hh"

namespace neutrino {

    enum scene_manager_events : std::int32_t {
        PUSH_SCENE,
        POP_SCENE,
        REPLACE_SCENE,
        PIN_SCENE,
        UNPIN_SCENE
    };

    using events = custom_event<scene_manager_events>;

    static bool s_scenes_manager_initialized = false;

    // Pending transitions outlive a consumed event when an application replaces
    // scene dispatch. Keep ownership here until dispatch or finish(), not in SDL.
    static std::mutex s_pending_mutex;
    static std::unordered_map <base_scene*, std::unique_ptr <base_scene>> s_pending_scenes;

    static std::unique_ptr <base_scene> take_pending_scene(void* id) {
        const std::lock_guard lock(s_pending_mutex);
        auto node = s_pending_scenes.extract(static_cast <base_scene*>(id));
        return node.empty() ? nullptr : std::move(node.mapped());
    }


    // Runs @p call — a call into @p scene — with the scene's painter state current (R48): what
    // it sets stays the scene's, and a push it leaves fails, naming @p what, after it is
    // undone. The state current before comes back afterwards, also when the call throws.
    template<class Call>
    static void as_current(base_scene* scene, const char* what, Call&& call) {
        detail::painter_state_stack::current_scope current(
            *detail::scene_context_access::painter_state_stack_of(scene));
        std::forward <Call>(call)();
        current.finish(what);
    }

    static void queue_scene(scene_manager_events code, std::unique_ptr <base_scene> scene) {
        ENFORCE(scene != nullptr);
        auto* scene_ptr = scene.get();
        {
            const std::lock_guard lock(s_pending_mutex);
            s_pending_scenes.emplace(scene.get(), std::move(scene));
        }
        try {
            events::push(code, scene_ptr);
        } catch (...) {
            auto discarded = take_pending_scene(scene_ptr);
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
        events::push(POP_SCENE, nullptr, nullptr);
    }

    void scenes_manager::pin_scene(std::unique_ptr <base_scene>&& scene) {
        queue_scene(PIN_SCENE, std::move(scene));
    }

    void scenes_manager::unpin_scene() {
        events::push(UNPIN_SCENE, nullptr, nullptr);
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
        events::drain();
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
                base_scene* const scene = m_stack.back().get();
                as_current(scene, "on_exit", [scene] { scene->on_exit(); });
            } catch (const std::exception& e) {
                LOG_ERROR("Scene on_exit threw during shutdown:", e.what());
            } catch (...) {
                LOG_ERROR("Scene on_exit threw an unknown exception during shutdown");
            }
            m_stack.pop_back(); // pop regardless: the stack must be empty when finish() returns
        }
        m_pinned = false;
    }

    void scenes_manager::register_events() {
        events::init();
    }

    void scenes_manager::setup_actions() {
        detail::adopt_pending_actions();
    }

    void scenes_manager::release_actions() {
        detail::return_actions();
    }

    std::optional <input_snapshot> scenes_manager::fire_global_actions(const input_snapshot& live) {
        return detail::current_action_table().fire_globals(live);
    }

    std::size_t scenes_manager::active_count() const noexcept {
        return m_stack.size() - (m_pinned ? 1 : 0);
    }

    base_scene* scenes_manager::active_top() const noexcept {
        const std::size_t n = active_count();
        return n == 0 ? nullptr : m_stack[n - 1].get();
    }

    blend_mode scenes_manager::scene_blend(const base_scene* scene) {
        // The scene's flags are the only source (painter.md §2.3, R45): a full-screen scene is
        // a plain copy, an overlay is blended over what is below. A transparent window shows
        // what its alpha says: a none copy would leave see-through what the scene left empty,
        // where the cleared window was opaque black, so there every scene blends.
        const sdlpp::window* window = service_locator::instance().maybe_window(); // sdlpp-ok: internal
        const bool see_through = window != nullptr
                && (window->get_flags() & sdlpp::window_flags::transparent) != sdlpp::window_flags::none; // sdlpp-ok: internal
        return has(scene->flags(), scene_flags::hides_below) && !see_through ? blend_mode::none : blend_mode::blend;
    }

    bool scenes_manager::enter_or_roll_back(base_scene* scene) {
        // Isolate the new scene's on_enter. If initialization throws (missing
        // asset, bad data, etc.) we roll back cleanly: give the partially-
        // entered scene a chance to clean up and drop it from the stack. The
        // app stays responsive instead of letting the exception escape to
        // SDL, which would turn it into SDL_APP_FAILURE and terminate.
        try {
            as_current(scene, "on_enter", [scene] { scene->on_enter(); });
            return true;
        } catch (const std::exception& ex) {
            LOG_ERROR("Scene on_enter threw:", ex.what());
        } catch (...) {
            LOG_ERROR("Scene on_enter threw an unknown exception");
        }
        try { as_current(scene, "on_exit", [scene] { scene->on_exit(); }); } catch (...) {}
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
            as_current(below, "on_pause", [below] { below->on_pause(); });
        }
        const auto at = m_stack.begin() + static_cast <std::ptrdiff_t>(active_count());
        base_scene* const entered = m_stack.insert(at, std::move(scene))->get();
        if (!enter_or_roll_back(entered) && pauses) {
            if (base_scene* below = active_top()) {
                try { as_current(below, "on_resume", [below] { below->on_resume(); }); } catch (...) {}
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
        base_scene* const leaving = m_stack[top].get();
        as_current(leaving, "on_exit", [leaving] { leaving->on_exit(); });
        m_stack.erase(m_stack.begin() + static_cast <std::ptrdiff_t>(top));
        // Mirror apply_push: only a scene that paused the one below resumes it.
        if (base_scene* below = active_top(); pauses && below) {
            as_current(below, "on_resume", [below] { below->on_resume(); });
        }
        notify_top_resized();
    }

    void scenes_manager::apply_replace(std::unique_ptr <base_scene>&& scene) {
        ENFORCE(active_count() > 0, "replace_scene: the stack has no scene to replace");
        auto& slot = m_stack[active_count() - 1];
        base_scene* const leaving = slot.get();
        as_current(leaving, "on_exit", [leaving] { leaving->on_exit(); });
        slot = std::move(scene);
        base_scene* const entering = slot.get();
        as_current(entering, "on_enter", [entering] { entering->on_enter(); });
        notify_top_resized();
    }

    void scenes_manager::apply_pin(std::unique_ptr <base_scene>&& scene) {
        apply_unpin(); // pinning replaces the pinned scene, if any
        const bool pauses = has(scene->flags(), scene_flags::pauses_below);
        if (base_scene* below = active_top(); pauses && below) {
            as_current(below, "on_pause", [below] { below->on_pause(); });
        }
        m_stack.emplace_back(std::move(scene));
        m_pinned = true;
        base_scene* const pinned = m_stack.back().get();
        if (enter_or_roll_back(pinned)) {
            const dim size = render_size();
            as_current(pinned, "on_resize", [pinned, size] { pinned->on_resize(size); });
            return;
        }
        m_pinned = false;
        if (base_scene* below = active_top(); pauses && below) {
            try { as_current(below, "on_resume", [below] { below->on_resume(); }); } catch (...) {}
        }
    }

    void scenes_manager::apply_unpin() {
        if (!m_pinned) {
            return;
        }
        const bool pauses = has(m_stack.back()->flags(), scene_flags::pauses_below);
        base_scene* const leaving = m_stack.back().get();
        as_current(leaving, "on_exit", [leaving] { leaving->on_exit(); });
        m_stack.pop_back();
        m_pinned = false;
        if (base_scene* below = active_top(); pauses && below) {
            as_current(below, "on_resume", [below] { below->on_resume(); });
        }
    }

    void scenes_manager::notify_top_resized() {
        if (base_scene* top = active_top()) {
            const dim size = render_size();
            as_current(top, "on_resize", [top, size] { top->on_resize(size); });
        }
    }

    void scenes_manager::notify_resize(dim size) {
        if (base_scene* top = active_top()) {
            as_current(top, "on_resize", [top, size] { top->on_resize(size); });
        }
        if (m_pinned) {
            base_scene* const pinned = m_stack.back().get();
            as_current(pinned, "on_resize", [pinned, size] { pinned->on_resize(size); });
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
        // scene gets no input. Raw events follow the same chain (handle_event). Every other
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
            base_scene* const scene = m_stack[i].get();
            const input_snapshot& input = i >= floor ? in : no_input;
            as_current(scene, "fixed_update", [scene, dt, &input] { scene->fixed_update(dt, input); });
        }
    }

    scenes_manager::frame_plan scenes_manager::plan() const {
        frame_plan p;
        if (m_stack.empty()) {
            return p;
        }
        // Every scene from the topmost hides_below one upward is drawn, bottom to top.
        for (size_t i = m_stack.size(); i-- > 0;) {
            if (has(m_stack[i]->flags(), scene_flags::hides_below)) {
                p.bottom = i;
                break;
            }
        }
        base_scene* const bottom = m_stack[p.bottom].get();
        if (!has(bottom->flags(), scene_flags::hides_below)) {
            return p;                          // an overlay at the bottom: through its canvas
        }
        const detail::canvas* canvas = detail::scene_context_access::canvas_of(bottom);
        if (!canvas) {
            return p;
        }
        // Straight into the target (R44) when the canvas would only copy the scene: no layers,
        // an every-frame canvas, and nothing above that can read below() — only a compositor
        // of a scene's own can. A canvas is always copied as it is (R45).
        bool direct = detail::shortcuts().bottom_scene_into_target
                      && detail::scene_context_access::layer_stack_of(bottom) == nullptr
                      && canvas->redraw() == redraw_mode::every_frame;
        for (size_t i = p.bottom + 1; direct && i < m_stack.size(); i++) {
            base_scene* const above = m_stack[i].get();
            if (detail::scene_context_access::layer_stack_of(above) != nullptr
                && !detail::is_default_compositor(*detail::scene_context_access::compositor_of(above))) {
                direct = false;
            }
        }
        p.direct = direct;
        // Otherwise its none copy writes every pixel of the target.
        p.covers = !direct && scene_blend(bottom) == blend_mode::none;
        return p;
    }

    bool scenes_manager::covers_target() const {
        return plan().covers;
    }

    void scenes_manager::render() {
        // Nothing is left current from before the frame, nor after it (painter.md §3.1): what
        // a scope that ended out of order left is reset, logged and counted.
        struct idle_check {
            idle_check() noexcept { detail::check_idle("before a frame"); }
            ~idle_check() { detail::check_idle("after a frame"); }
            idle_check(const idle_check&) = delete;
            idle_check& operator=(const idle_check&) = delete;
        } const idle;

        // The target is cleared here, once, to black — unless the bottom scene's canvas will
        // cover it with an opaque copy (R44). Then it is cleared only if nothing ends up
        // covering it, or if that scene has no layers, its canvas could not be made and its
        // render() draws straight into the target: until the copies, nothing else is drawn
        // there.
        auto& r = get_renderer();
        bool cleared = false;
        const auto clear_once = [&] {
            if (!cleared) {
                cleared = true;
                const sdlpp::renderer::draw_color_guard black(r, colors::black); // sdlpp-ok: internal
                (void) r.clear();
            }
        };
        const auto bottom_shows = [this] {
            return !m_finished.empty() && m_finished.front() && m_finished.front()->has_content();
        };

        // The canvases drawn so far, bottom to top: what each scene's compositor sees below,
        // and what is drawn into the target at the end. Members, so a frame allocates nothing.
        std::vector <const texture*>& below = m_below;
        std::vector <detail::canvas*>& finished = m_finished;
        below.clear();
        finished.clear();
        try {
            const frame_plan p = plan();
            if (!p.covers) {
                clear_once();
            }
            if (m_stack.empty()) {
                return;
            }
            const size_t index = p.bottom;

            // A scene hidden under one with hides_below gives back the textures of its
            // every-frame canvas and layers (R41), first, so the scenes drawn this frame can
            // reuse them.
            for (size_t i = 0; i < index; i++) {
                base_scene* const hidden = m_stack[i].get();
                if (detail::canvas* canvas = detail::scene_context_access::canvas_of(hidden)) {
                    canvas->release_every_frame();
                }
                if (detail::layer_stack* layers = detail::scene_context_access::layer_stack_of(hidden)) {
                    layers->release_every_frame();
                }
            }

            for (size_t i = index; i < m_stack.size(); i++) {
                base_scene* const scene = m_stack[i].get();
                {
                    // The scene draws onto its canvas, or into its layers, which its compositor
                    // then combines onto the canvas; the free push_layer / pop_layer reach them
                    // meanwhile. The bottom scene may draw straight into the target (R44), and
                    // does if its canvas cannot be made: its picture is the first there. Any
                    // other scene's failed canvas discards its drawing (painter.md §3.6). If
                    // render() or the compositor throws, the frame unwinds.
                    const auto onto = i != index ? detail::frame_target::canvas
                                      : p.direct ? detail::frame_target::bound_target
                                                 : detail::frame_target::canvas_or_bound_target;
                    detail::scene_frame frame(scene, below, onto);
                    if (i == index && frame.draws_into_target()) {
                        clear_once();          // its drawing lands in the target: clear it first
                    }
                    scene->render();
                    frame.finish();
                }
                detail::canvas* canvas = detail::scene_context_access::canvas_of(scene);
                if (canvas) {
                    canvas->set_blend(scene_blend(scene));
                }
                finished.push_back(canvas);
                if (canvas && canvas->has_content()) {
                    below.push_back(canvas->get());
                }
            }
        } catch (...) {
            // The scenes finished below the one that threw still show, as they did when they
            // drew straight into the window; the application then pops the scene.
            try {
                if (!bottom_shows()) {
                    clear_once();
                }
                detail::draw_canvases(finished);
            } catch (const std::exception& e) {
                LOG_ERROR("cannot draw the scenes below a scene that threw:", e.what());
            } catch (...) {
                LOG_ERROR("cannot draw the scenes below a scene that threw");
            }
            throw;
        }

        // The canvases into the target bound now, bottom to top (painter.md §3.2).
        if (!bottom_shows()) {
            clear_once();
        }
        detail::draw_canvases(finished);
    }

    void scenes_manager::handle_event(const sdlpp::event& ev) {
        if (sdlpp::event_registry::is_custom(ev.type())) {
            auto* ue = ev.as <sdlpp::user_event>();
            auto type = static_cast <uint32_t>(ue->type);
            if (type == events::category()) {
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
            // before delivering, so a handle_event that changes them routes the next event.
            for (size_t i = m_stack.size(); i-- > 0;) {
                base_scene* const scene = m_stack[i].get();
                const auto f = scene->flags();
                as_current(scene, "handle_event", [scene, &ev] { scene->handle_event(ev); });
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
