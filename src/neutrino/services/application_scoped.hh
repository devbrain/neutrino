//
// Created by igor on 17/11/2026.
//

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>

#include <failsafe/enforce.hh>

// Objects that live as long as the application: made on first use, destroyed when the
// application quits, newest first, while SDL is still up. Borrowed from bsw's singleton /
// register_at_exit and changed for neutrino: the scope is the application, not the process,
// so a test can run many applications one after another.

namespace neutrino::detail {
    /**
     * @brief Opens the application scope: application_scoped objects may be made from now
     *        on. Called by application::on_ready(), before the services are made.
     */
    void open_application_scope() noexcept;

    /**
     * @brief Closes the application scope: destroys everything registered with at_quit(),
     *        newest first, and refuses new application_scoped objects until the scope is
     *        opened again.
     *
     * Called by application::on_quit(), after the scenes and teardown() and before the
     * application's own services and SDL go. A callback that throws is logged and the others
     * still run. Calling it again, or with nothing registered, does nothing.
     */
    void close_application_scope() noexcept;

    /// @brief True between open_application_scope() and close_application_scope().
    [[nodiscard]] bool application_scope_is_open() noexcept;

    /**
     * @brief Registers @p fn to run when the application scope closes. Callbacks run newest
     *        first, so something registered later — which may depend on what was registered
     *        earlier — goes first. Any thread. ENFORCE: the scope is open, checked under the
     *        lock close_application_scope() closes it under, so a callback is never left
     *        behind for the next application.
     */
    void at_quit(std::function <void()> fn);

    /**
     * @brief The one @p T of the running application, made on first use and destroyed when
     *        the application quits (close_application_scope()).
     *
     * Destruction runs newest first: a @p T whose constructor uses another
     * application_scoped object makes that one first, so it is destroyed after @p T. After
     * the application quits, maybe() returns null, and the next application gets a new @p T.
     *
     * instance() may be called from any thread; the first call makes @p T exactly once. It
     * fails ENFORCE outside an open scope — before on_ready() or once on_quit() has begun —
     * so nothing is made after SDL is gone. Destructors, and anything else that may run
     * during or after quit, use maybe().
     *
     * @tparam T default-constructible. Internal: only the engine's own types use it.
     */
    template<typename T>
    class application_scoped {
        public:
            application_scoped() = delete;

            /// @brief The application's @p T, made now if it does not exist yet.
            ///        ENFORCE: the application scope is open.
            [[nodiscard]] static T& instance() {
                if (T* p = s_object.load(std::memory_order_acquire)) {
                    return *p;
                }
                return create();
            }

            /// @brief The application's @p T, or null if it has not been made, or the
            ///        application has quit.
            [[nodiscard]] static T* maybe() noexcept {
                return s_object.load(std::memory_order_acquire);
            }

        private:
            static T& create() {
                const std::lock_guard lock(s_mutex);
                if (T* p = s_object.load(std::memory_order_relaxed)) {
                    return *p;                       // another thread made it meanwhile
                }
                ENFORCE(application_scope_is_open(),
                        "an application_scoped object is used outside the application: before "
                        "on_ready() or after on_quit() began");
                auto owned = std::make_unique <T>();
                // Fails if the scope closed since the check above; owned is destroyed then.
                at_quit([] { destroy(); });
                T* p = owned.release();
                s_object.store(p, std::memory_order_release);
                return *p;
            }

            static void destroy() noexcept {
                T* p = nullptr;
                {
                    const std::lock_guard lock(s_mutex);
                    p = s_object.exchange(nullptr, std::memory_order_acq_rel);
                }
                // Outside the lock, and after maybe() already answers null: the destructor may
                // look at other application_scoped objects, or at this one, through maybe().
                delete p;
            }

            inline static std::atomic <T*> s_object{nullptr};
            inline static std::mutex s_mutex;
    };
}
