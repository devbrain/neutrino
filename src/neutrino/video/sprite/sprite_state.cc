//
// Created by igor on 05/07/2026.
//

#include <neutrino/video/sprite/sprite_state.hh>

#include "services/service_access.hh"
#include "video/sprite/sprites_manager.hh"

namespace neutrino {
    sprite_state_id create_sprite_state(sprite_appearance appearance) {
        return require_sprites_manager().create(appearance);
    }

    sprite_state_id create_sprite_state(sprite_animation_id animation) {
        return require_sprites_manager().create(animation);
    }

    void set_sprite_state_appearance(sprite_state_id state, sprite_appearance appearance) {
        require_sprites_manager().set_appearance(state, appearance);
    }

    void restart_sprite_animation(sprite_state_id state, sprite_animation_id animation) {
        require_sprites_manager().set_animation(state, animation);
    }

    bool switch_sprite_animation(sprite_state_id state, sprite_animation_id animation) {
        return require_sprites_manager().switch_animation(state, animation);
    }

    sprite_appearance sprite_state_appearance(sprite_state_id state) {
        return require_sprites_manager().appearance(state);
    }

    bool sprite_state_finished(sprite_state_id state) {
        return require_sprites_manager().finished(state);
    }

    void unregister_sprite_state(sprite_state_id state) {
        if (!state.valid()) {
            return;
        }
        // Services already torn down: the resource is gone, nothing to do.
        if (auto* manager = maybe_sprites_manager()) {
            manager->erase(state);
        }
    }
}
