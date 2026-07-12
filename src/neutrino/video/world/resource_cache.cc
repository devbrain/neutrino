//
// Created by igor on 07/07/2026.
//

#include <neutrino/video/world/resource_cache.hh>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#include <neutrino/detail/hash.hh>
#include <neutrino/video/sprite/image_identity.hh>

#include "video/sprite/resource_cache_core.hh"

namespace neutrino {
    struct resource_cache::impl {
        explicit impl(std::size_t cold_budget)
            : core(cold_budget) {
        }

        resource_cache_core <tileset_bundle> core;
        image_identifier                     identifier;

        // The cache key: image identities folded with the grid geometry and per-tile
        // images/animations. `first_gid` is deliberately excluded -- it is a map-level
        // global-id assignment, not tileset content, so the same tileset shares one
        // bundle across maps that number it differently.
        content_key key_for(const world_tileset& ts) {
            std::uint64_t digest = 0;
            std::uint64_t length = 0;

            const auto fold_u = [&] (std::uint64_t v) {
                details::hash_combine64(digest, v);
            };
            const auto fold_image = [&] (const world_image& img) {
                fold_image_identity(digest, length, identifier, img);
            };

            if (ts.image) {
                fold_image(*ts.image);
            }
            fold_u(ts.tile_width);
            fold_u(ts.tile_height);
            fold_u(ts.spacing);
            fold_u(ts.margin);
            fold_u(ts.columns);
            fold_u(ts.tile_count);
            fold_u(static_cast <std::uint32_t>(ts.offset_x));
            fold_u(static_cast <std::uint32_t>(ts.offset_y));

            for (const world_tile& t : ts.tiles) {
                fold_u(t.id);
                if (t.image) {
                    fold_image(*t.image);
                }
                // The sub-rectangle an image-collection tile slices from its image is
                // what world_tileset::drawable renders. Fold its presence and bounds, or
                // two tilesets sharing one atlas image but slicing different regions would
                // collide on the same key and serve the wrong tile after a cache hit.
                fold_u(t.source_rect.has_value() ? 1u : 0u);
                if (t.source_rect) {
                    fold_u(static_cast <std::uint32_t>(t.source_rect->x));
                    fold_u(static_cast <std::uint32_t>(t.source_rect->y));
                    fold_u(static_cast <std::uint32_t>(t.source_rect->w));
                    fold_u(static_cast <std::uint32_t>(t.source_rect->h));
                }
                for (const world_tile_animation_frame& f : t.animation) {
                    fold_u(f.tile);
                    fold_u(static_cast <std::uint64_t>(f.duration.count()));
                }
            }

            return content_key{digest, length};
        }
    };

    resource_cache::resource_cache(std::size_t cold_budget)
        : m_impl(std::make_unique <impl>(cold_budget)) {
    }

    // Destroying the impl drops the core's entries, whose bundles unregister themselves.
    resource_cache::~resource_cache() = default;

    bundle_handle resource_cache::acquire(const world_tileset& tileset) {
        const content_key key = m_impl->key_for(tileset);
        // build runs only on a miss; a throwing build leaves the cache untouched.
        const auto h = m_impl->core.acquire(key, [&] { return build_bundle(tileset); });
        return bundle_handle{h.key, h.bundle, h.token};
    }

    void resource_cache::release(const bundle_handle& handle) {
        m_impl->core.release(
            resource_cache_core <tileset_bundle>::handle{handle.key, handle.bundle, handle.token});
    }

    std::size_t resource_cache::resident_count() const noexcept {
        return m_impl->core.resident_count();
    }

    std::size_t resource_cache::cold_count() const noexcept {
        return m_impl->core.cold_count();
    }
}
