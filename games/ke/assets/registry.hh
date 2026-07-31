//
// Game-wide built render assets, published through a KE-local service accessor so
// define_sprites / the scene / the actors layer reach them without threading.
//

#pragma once

#include <neutrino/video/sprite/sprite_cache.hh>
#include <ke/resources/resources.hh>
#include <ke/resources/cell.hh>

namespace rs {
    // Level-invariant render assets: the leased sprite sets drawn through the sprite_batch
    // (actors + the backdrop's walls/fill), and the cache they lease from. Each set answers
    // its own frame geometry (frame_rect / origin), so no source sprite_def is retained.
    // Owned by the scene, populated by define_sprites(), published via set_ke_assets.
    // Non-copyable/non-movable (holds a live sprite_cache), so it is constructed in place.
    struct ke_assets {
        neutrino::sprite_cache      cache;   ///< sprites lease their built sets from here

        neutrino::sprite_set_handle paddle;  ///< the built + leased KE_RACK paddle set
        neutrino::sprite_set_handle bricks;  ///< the built + leased KE_BRICK set
        neutrino::sprite_set_handle balls;   ///< the built + leased KE_SPELL ball set
        neutrino::sprite_set_handle board;   ///< KE_BORD wall pillars (backdrop, top-left pivot)
        neutrino::sprite_set_handle fill;    ///< KE_FILL score/fill tiles (backdrop, top-left pivot)

        std::vector<ke_level>       levels;

        neutrino::sprite_animation_id hit_wall_anim_id;  // sparks when the ball hits the wall
        neutrino::sprite_animation_id hit_brick_anim_id; // sparks when the ball hits the brick

        std::array<neutrino::sprite_animation_id, 28> capsule_anim_id;

        game_resources*             m_resources{nullptr};

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
