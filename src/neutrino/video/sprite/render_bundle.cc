//
// Created by igor on 12/07/2026.
//

#include <neutrino/video/sprite/render_bundle.hh>

namespace neutrino {
    void render_bundle::release() noexcept {
        for (const sprite_state_id state : states) {
            unregister_sprite_state(state);
        }
        for (const sprite_animation_id animation : animations) {
            unregister_sprite_animation(animation);
        }
        for (const sprite_sheet_id sheet : sheets) {
            unregister_sprite_sheet(sheet);
        }
        for (const gpu_texture_atlas_id atlas : atlases) {
            unregister_atlas(atlas);
        }
        // Clear now that every id is unregistered, so a repeat call (or the destructor)
        // iterates empty vectors and does nothing.
        states.clear();
        animations.clear();
        sheets.clear();
        atlases.clear();
    }

    // RAII: the destructor performs the same dependency-ordered teardown, so every build
    // local and every cache entry releases its registered resources exactly once.
    render_bundle::~render_bundle() {
        release();
    }

    // Swap our (empty, freshly default-constructed) vectors with the source's, so the
    // source is left provably empty by exchange semantics -- not by relying on the
    // vector's allocator-dependent moved-from state -- and its destructor no-ops.
    render_bundle::render_bundle(render_bundle&& other) noexcept {
        atlases.swap(other.atlases);
        sheets.swap(other.sheets);
        animations.swap(other.animations);
        states.swap(other.states);
    }

    // Release our own resources first (release leaves our vectors empty), then swap the
    // source in, so overwriting a live bundle can never leak and the source no-ops.
    render_bundle& render_bundle::operator=(render_bundle&& other) noexcept {
        if (this != &other) {
            release();
            atlases.swap(other.atlases);
            sheets.swap(other.sheets);
            animations.swap(other.animations);
            states.swap(other.states);
        }
        return *this;
    }
}
