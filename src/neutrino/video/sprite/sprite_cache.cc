//
// Created by igor on 12/07/2026.
//

#include <neutrino/video/sprite/sprite_cache.hh>

#include <cstddef>
#include <memory>
#include <utility>

#include <failsafe/enforce.hh>

#include <neutrino/video/sprite/image_identity.hh>

#include "video/sprite/resource_cache_core.hh"

namespace neutrino {
    namespace {
        // An empty lease answers every optional lookup with nullopt, so a require_* on one would
        // otherwise report "no such frame" for a set that was simply never acquired. Separate the
        // two failures: this one means the HANDLE is wrong, not the key.
        void enforce_leased(const void* set, const char* what) {
            ENFORCE(set != nullptr)("sprite_set_handle: ", what, " on an empty lease (no set acquired)");
        }
    } // namespace

    // --- sprite_set_handle: a lease over sprite_cache's retain/release primitives ---

    sprite_visual_ref sprite_set_handle::require_visual(std::string_view name) const {
        enforce_leased(m_set, "require_visual");
        return m_set->require_visual(name);
    }

    sprite_visual_ref sprite_set_handle::require_visual(std::size_t index) const {
        enforce_leased(m_set, "require_visual");
        return m_set->require_visual(index);
    }

    rect sprite_set_handle::require_frame_rect(std::string_view name) const {
        enforce_leased(m_set, "require_frame_rect");
        return m_set->require_frame_rect(name);
    }

    rect sprite_set_handle::require_frame_rect(std::size_t index) const {
        enforce_leased(m_set, "require_frame_rect");
        return m_set->require_frame_rect(index);
    }

    point sprite_set_handle::require_origin(std::string_view name) const {
        enforce_leased(m_set, "require_origin");
        return m_set->require_origin(name);
    }

    point sprite_set_handle::require_origin(std::size_t index) const {
        enforce_leased(m_set, "require_origin");
        return m_set->require_origin(index);
    }

    sprite_metrics sprite_set_handle::require_metrics(std::string_view name) const {
        enforce_leased(m_set, "require_metrics");
        return m_set->require_metrics(name);
    }

    sprite_metrics sprite_set_handle::require_metrics(std::size_t index) const {
        enforce_leased(m_set, "require_metrics");
        return m_set->require_metrics(index);
    }

    sprite_animation_id sprite_set_handle::require_clip(std::string_view name) const {
        enforce_leased(m_set, "require_clip");
        return m_set->require_clip(name);
    }

    sprite_set_handle::sprite_set_handle(const sprite_set_handle& other)
        : m_cache(other.m_cache), m_key(other.m_key), m_token(other.m_token), m_set(other.m_set) {
        if (m_cache && m_set) {
            m_cache->retain(*this);
        }
    }

    sprite_set_handle& sprite_set_handle::operator=(const sprite_set_handle& other) {
        if (this != &other) {
            if (other.m_cache && other.m_set) {
                other.m_cache->retain(other); // retain the new entry before releasing the old
            }
            if (m_cache && m_set) {
                m_cache->release(*this);
            }
            m_cache = other.m_cache;
            m_key   = other.m_key;
            m_token = other.m_token;
            m_set   = other.m_set;
        }
        return *this;
    }

    sprite_set_handle::sprite_set_handle(sprite_set_handle&& other) noexcept
        : m_cache(other.m_cache), m_key(other.m_key), m_token(other.m_token), m_set(other.m_set) {
        other.m_cache = nullptr;
        other.m_set   = nullptr;
        other.m_token = 0;
        other.m_key   = content_key{};
    }

    sprite_set_handle& sprite_set_handle::operator=(sprite_set_handle&& other) noexcept {
        if (this != &other) {
            if (m_cache && m_set) {
                m_cache->release(*this);
            }
            m_cache = other.m_cache;
            m_key   = other.m_key;
            m_token = other.m_token;
            m_set   = other.m_set;
            other.m_cache = nullptr;
            other.m_set   = nullptr;
            other.m_token = 0;
            other.m_key   = content_key{};
        }
        return *this;
    }

    sprite_set_handle::~sprite_set_handle() {
        if (m_cache && m_set) {
            m_cache->release(*this);
        }
    }

    sprite_instance sprite_set_handle::spawn(std::string_view name) const {
        const auto anim = clip(name);
        if (!anim) {
            return sprite_instance{};
        }
        // The instance takes a *copy* of this lease (retain) so its set stays resident.
        return sprite_instance{*this, create_sprite_state(*anim)};
    }

    // --- sprite_instance: owns a state + a lease on its set ---

    sprite_instance::~sprite_instance() {
        // Unregister the state first, while its animation is still resident via the lease;
        // then the m_lease member destructs and releases (which may cool/evict the set).
        if (m_state.valid()) {
            unregister_sprite_state(m_state);
        }
    }

    sprite_instance::sprite_instance(sprite_instance&& other) noexcept
        : m_lease(std::move(other.m_lease)), m_state(other.m_state) {
        other.m_state = sprite_state_id{};
    }

    sprite_instance& sprite_instance::operator=(sprite_instance&& other) noexcept {
        if (this != &other) {
            // Release our own state (its animation still resident via our old lease) before
            // the lease move-assign drops that lease.
            if (m_state.valid()) {
                unregister_sprite_state(m_state);
            }
            m_lease = std::move(other.m_lease);
            m_state = other.m_state;
            other.m_state = sprite_state_id{};
        }
        return *this;
    }

    bool sprite_instance::switch_to(std::string_view name) {
        const auto anim = m_lease.clip(name);
        if (!anim || !m_state.valid()) {
            return false;
        }
        return switch_sprite_animation(m_state, *anim);
    }

    bool sprite_instance::restart(std::string_view name) {
        const auto anim = m_lease.clip(name);
        if (!anim || !m_state.valid()) {
            return false;
        }
        restart_sprite_animation(m_state, *anim);
        return true;
    }

    // --- sprite_cache ---

    struct sprite_cache::impl {
        explicit impl(std::size_t cold_budget)
            : core(cold_budget) {
        }

        resource_cache_core <sprite_set> core;
        image_identifier                 identifier;
    };

    sprite_cache::sprite_cache(std::size_t cold_budget)
        : m_impl(std::make_unique <impl>(cold_budget)) {
    }

    // Destroying the impl drops the core's entries, whose sprite_sets unregister themselves.
    sprite_cache::~sprite_cache() = default;

    sprite_set_handle sprite_cache::acquire(const sprite_def& def) {
        const content_key key = key_for(def, m_impl->identifier);
        // build runs only on a miss; a throwing build leaves the cache untouched.
        const auto h = m_impl->core.acquire(key, [&] { return build_sprite_set(def); });
        return sprite_set_handle{this, h.key, h.token, h.bundle}; // adopts acquire's refcount
    }

    void sprite_cache::retain(const sprite_set_handle& handle) {
        m_impl->core.retain(
            resource_cache_core <sprite_set>::handle{handle.m_key, handle.m_set, handle.m_token});
    }

    void sprite_cache::release(const sprite_set_handle& handle) {
        m_impl->core.release(
            resource_cache_core <sprite_set>::handle{handle.m_key, handle.m_set, handle.m_token});
    }

    std::size_t sprite_cache::resident_count() const noexcept {
        return m_impl->core.resident_count();
    }

    std::size_t sprite_cache::cold_count() const noexcept {
        return m_impl->core.cold_count();
    }
}
