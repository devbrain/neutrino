//
// Created by igor on 05/07/2026.
//

#include <neutrino/video/sprite/sprite_state.hh>

#include <utility>

#include <failsafe/enforce.hh>

#include "services/service_locator.hh"
#include "video/sprite/sprites_manager.hh"

namespace neutrino {
    sprite_state_id create_sprite_state(sprite_appearance appearance) {
        auto* manager = service_locator::instance().get_sprites_manager();
        ENFORCE(manager != nullptr);
        return manager->create(appearance);
    }

    sprite_state_id create_sprite_state(sprite_animation_id animation) {
        auto* manager = service_locator::instance().get_sprites_manager();
        ENFORCE(manager != nullptr);
        return manager->create(animation);
    }

    void set_sprite_state_appearance(sprite_state_id state, sprite_appearance appearance) {
        auto* manager = service_locator::instance().get_sprites_manager();
        ENFORCE(manager != nullptr);
        manager->set_appearance(state, appearance);
    }

    void set_sprite_state_animation(sprite_state_id state, sprite_animation_id animation) {
        auto* manager = service_locator::instance().get_sprites_manager();
        ENFORCE(manager != nullptr);
        manager->set_animation(state, animation);
    }

    sprite_appearance sprite_state_appearance(sprite_state_id state) {
        auto* manager = service_locator::instance().get_sprites_manager();
        ENFORCE(manager != nullptr);
        return manager->appearance(state);
    }

    bool sprite_state_finished(sprite_state_id state) {
        auto* manager = service_locator::instance().get_sprites_manager();
        ENFORCE(manager != nullptr);
        return manager->finished(state);
    }

    void unregister_sprite_state(sprite_state_id state) {
        if (!state.valid()) {
            return;
        }

        auto* manager = service_locator::instance().get_sprites_manager();
        ENFORCE(manager != nullptr);
        manager->erase(state);
    }
}
