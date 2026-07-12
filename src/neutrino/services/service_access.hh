//
// Created by igor on 12/07/2026.
//

#pragma once

//
// Internal accessors for the application-scoped services. `require_X()` returns a
// reference and hard-fails if the service is absent (a programming/ordering error --
// the application must be ready). `maybe_X()` returns a possibly-null pointer for
// shutdown-safe paths, where "service gone" means "nothing to do". These collapse the
// `service_locator::instance().get_X()` + null-check idiom that recurred across the
// sprite, audio, and input code into one call.
//

#include <failsafe/enforce.hh>

#include "services/service_locator.hh"

namespace neutrino {
    class sound_system; // fully declared where used; service_locator forward-declares it
    class application;

    [[nodiscard]] inline sprites_manager& require_sprites_manager() {
        auto* m = service_locator::instance().get_sprites_manager();
        ENFORCE(m != nullptr)("sprites_manager is not available; is the application ready?");
        return *m;
    }
    [[nodiscard]] inline sprites_manager* maybe_sprites_manager() noexcept {
        return service_locator::instance().get_sprites_manager();
    }

    [[nodiscard]] inline texture_registry& require_texture_registry() {
        auto* r = service_locator::instance().get_texture_registry();
        ENFORCE(r != nullptr)("texture_registry is not available; is the application ready?");
        return *r;
    }
    [[nodiscard]] inline texture_registry* maybe_texture_registry() noexcept {
        return service_locator::instance().get_texture_registry();
    }

    [[nodiscard]] inline sound_system& require_sound_system() {
        auto* s = service_locator::instance().get_sound_system();
        ENFORCE(s != nullptr)("sound_system is not available; is the application ready?");
        return *s;
    }
    [[nodiscard]] inline sound_system* maybe_sound_system() noexcept {
        return service_locator::instance().get_sound_system();
    }

    [[nodiscard]] inline application* maybe_application() noexcept {
        return service_locator::instance().get_application();
    }

    [[nodiscard]] inline gamepads* maybe_gamepads() noexcept {
        return service_locator::instance().get_gamepads();
    }

    /// @brief Shutdown-safe unregister: erase @p id from @p manager when the id is live and
    ///        the manager still exists ("already torn down" means nothing to do).
    template <class Id, class Manager>
    inline void erase_if_live(Id id, Manager* manager) {
        if (id.valid() && manager != nullptr) {
            manager->erase(id);
        }
    }
}
