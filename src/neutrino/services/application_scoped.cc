//
// Created by igor on 17/11/2026.
//

#include <exception>
#include <utility>
#include <vector>

#include <failsafe/enforce.hh>
#include <failsafe/logger.hh>

#include "services/application_scoped.hh"

namespace neutrino::detail {
    namespace {
        struct registry {
            std::mutex mutex;
            std::vector <std::function <void()>> callbacks;   // run from the back
            std::atomic <bool> open{false};
        };

        // A function-local static: made on first use, so it exists before any
        // application_scoped object registers with it.
        registry& get_registry() {
            static registry r;
            return r;
        }
    }

    void open_application_scope() noexcept {
        auto& r = get_registry();
        const std::lock_guard lock(r.mutex);
        r.open.store(true, std::memory_order_release);
    }

    void close_application_scope() noexcept {
        auto& r = get_registry();
        {
            // Under the lock at_quit() checks it under, so nothing registers after it.
            const std::lock_guard lock(r.mutex);
            r.open.store(false, std::memory_order_release);
        }
        // One callback at a time, outside the lock: a destructor may look at other objects.
        for (;;) {
            std::function <void()> fn;
            {
                const std::lock_guard lock(r.mutex);
                if (r.callbacks.empty()) {
                    return;
                }
                fn = std::move(r.callbacks.back());
                r.callbacks.pop_back();
            }
            try {
                fn();
            } catch (const std::exception& e) {
                LOG_ERROR("an application-scoped destructor threw at quit:", e.what());
            } catch (...) {
                LOG_ERROR("an application-scoped destructor threw at quit");
            }
        }
    }

    bool application_scope_is_open() noexcept {
        return get_registry().open.load(std::memory_order_acquire);
    }

    void at_quit(std::function <void()> fn) {
        auto& r = get_registry();
        const std::lock_guard lock(r.mutex);
        // Checked here, under the lock close_application_scope() closes under: an object made
        // while the scope closed would otherwise never be destroyed, and the next application
        // would get it.
        ENFORCE(r.open.load(std::memory_order_relaxed),
                "at_quit after the application scope closed: on_quit() has begun");
        r.callbacks.push_back(std::move(fn));
    }
}
