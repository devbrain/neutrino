//
// Game-wide built render assets, published through a KE-local service accessor so
// define_sprites / build_world / the scene / the actors layer reach them without threading.
//

#pragma once

#include <neutrino/video/sprite/sprite_cache.hh>
#include <neutrino/video/sprite/sprite_def.hh>
#include <neutrino/world/world_tileset.hh>

namespace rs {
    // Level-invariant assets: the brick tileset, the actor sprite defs, and the cache their
    // instances lease from. (The background is per-level -- parametrized by fill_block -- so
    // it is built with each level's world, not stored here.) Owned by the scene, populated
    // by define_sprites(), published via set_ke_assets. Non-copyable/non-movable (holds a
    // live sprite_cache), so it is constructed in place and referenced.
    struct ke_assets {
        neutrino::sprite_cache      cache;      ///< actors lease their built sets from here
        neutrino::world_tileset     blocks;     ///< KE_BRICK collection tileset (world-drawn)
        neutrino::sprite_def        paddle_def; ///< KE_RACK sprite def (visuals with baked origins)
        neutrino::sprite_set_handle paddle;     ///< the built + leased paddle set

        ke_assets() = default;
        ke_assets(const ke_assets&) = delete;
        ke_assets& operator=(const ke_assets&) = delete;
    };

    // KE service accessor: the owner (scene) publishes a non-owning pointer; callers reach
    // the assets through it. Mirrors neutrino's service_locator pattern.
    void set_ke_assets(ke_assets& assets);
    [[nodiscard]] ke_assets& require_ke_assets();
    void clear_ke_assets();
}
