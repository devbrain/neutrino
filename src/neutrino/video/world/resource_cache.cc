//
// Created by igor on 07/07/2026.
//

#include <neutrino/video/world/resource_cache.hh>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include <failsafe/exception.hh>

#include <neutrino/detail/hash.hh>

#include "utils/lru.hh"

namespace neutrino {
    namespace {
        // Content identity of a raw image byte buffer.
        content_key hash_bytes_identity(const std::vector<std::uint8_t>& data) {
            return content_hash(std::as_bytes(std::span<const std::uint8_t>(data)));
        }

        // Content identity of a file's bytes (read whole; callers cache the result by
        // stat key so this only runs when the file is new or has changed).
        content_key hash_file_identity(const std::filesystem::path& path) {
            std::ifstream in(path, std::ios::binary);
            if (!in) {
                THROW_RUNTIME("resource_cache: cannot open image file: ", path.string());
            }
            const std::vector<std::uint8_t> bytes(
                (std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            return content_hash(std::as_bytes(std::span<const std::uint8_t>(bytes)));
        }
    } // namespace

    struct resource_cache::impl {
        struct entry {
            tileset_bundle bundle;
            int            refcount;
            std::uint64_t  token; ///< Distinguishes this entry from a later rebuild under the same key.
        };

        struct stat_id {
            std::filesystem::file_time_type mtime;
            std::uintmax_t                  size;
            content_key                     content;
        };

        explicit impl(std::size_t cold_budget)
            : cold(cold_budget) {
        }

        std::unordered_map<content_key, entry> entries;
        utils::lru_index<content_key>          cold;
        std::unordered_map<std::string, stat_id> stat_cache;
        std::uint64_t                          next_token = 1; ///< Monotonic; stamps each fresh build.

        // Identify one image: embedded bytes hash directly; a file is identified by
        // its (mtime, size) stat key mapped to a cached content hash, recomputed only
        // when the file changes. An image with no bytes contributes an empty identity.
        content_key image_identity(const world_image& img) {
            if (!img.data.empty()) {
                return hash_bytes_identity(img.data);
            }
            if (img.source.empty()) {
                return content_key{0, 0};
            }

            std::error_code ec;
            const auto mtime = std::filesystem::last_write_time(img.source, ec);
            const auto size = std::filesystem::file_size(img.source, ec);
            const std::string key = img.source.string();
            if (!ec) {
                const auto it = stat_cache.find(key);
                if (it != stat_cache.end() && it->second.mtime == mtime && it->second.size == size) {
                    return it->second.content;
                }
            }

            const content_key content = hash_file_identity(img.source);
            if (!ec) {
                stat_cache[key] = stat_id{mtime, size, content};
            }
            return content;
        }

        // The cache key: image identities folded with the grid geometry and per-tile
        // images/animations. `first_gid` is deliberately excluded — it is a map-level
        // global-id assignment, not tileset content, so the same tileset shares one
        // bundle across maps that number it differently.
        content_key key_for(const world_tileset& ts) {
            std::uint64_t digest = 0;
            std::uint64_t length = 0;

            const auto fold_u = [&] (std::uint64_t v) {
                details::hash_combine64(digest, v);
            };
            const auto fold_image = [&] (const world_image& img) {
                const content_key id = image_identity(img);
                details::hash_combine64(digest, id.hash);
                details::hash_combine64(digest, id.length);
                // Declared dimensions drive src-rect and column math in world_tileset
                // (tile_rect derives columns from image->width when columns == 0;
                // collection tiles take src straight from width/height). They can
                // diverge from the decoded bytes, so fold them alongside byte identity.
                details::hash_combine64(digest, img.width);
                details::hash_combine64(digest, img.height);
                length += id.length;
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
            fold_u(static_cast<std::uint32_t>(ts.offset_x));
            fold_u(static_cast<std::uint32_t>(ts.offset_y));

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
                    fold_u(static_cast<std::uint32_t>(t.source_rect->x));
                    fold_u(static_cast<std::uint32_t>(t.source_rect->y));
                    fold_u(static_cast<std::uint32_t>(t.source_rect->w));
                    fold_u(static_cast<std::uint32_t>(t.source_rect->h));
                }
                for (const world_tile_animation_frame& f : t.animation) {
                    fold_u(f.tile);
                    fold_u(static_cast<std::uint64_t>(f.duration.count()));
                }
            }

            return content_key{digest, length};
        }

        // Tear down and forget a cold entry (its refcount is zero, so no live handle
        // still points at it). Erasing the node runs the bundle's destructor, which
        // unregisters its resources.
        void evict(const content_key& key) {
            entries.erase(key);
        }
    };

    resource_cache::resource_cache(std::size_t cold_budget)
        : m_impl(std::make_unique<impl>(cold_budget)) {
    }

    // Destroying the impl drops the entries map, whose bundles unregister themselves.
    resource_cache::~resource_cache() = default;

    bundle_handle resource_cache::acquire(const world_tileset& tileset) {
        const content_key key = m_impl->key_for(tileset);

        if (const auto it = m_impl->entries.find(key); it != m_impl->entries.end()) {
            if (it->second.refcount == 0) {
                m_impl->cold.erase(key); // resurrect from the cold pool
            }
            ++it->second.refcount;
            return bundle_handle{key, &it->second.bundle, it->second.token};
        }

        // Miss: build before inserting, so a build failure leaves the cache untouched.
        tileset_bundle bundle = build_bundle(tileset);
        const std::uint64_t token = m_impl->next_token++;
        const auto [it, inserted] =
            m_impl->entries.emplace(key, impl::entry{std::move(bundle), 1, token});
        return bundle_handle{key, &it->second.bundle, token};
    }

    void resource_cache::release(const bundle_handle& handle) {
        if (!handle.valid()) {
            return;
        }
        const auto it = m_impl->entries.find(handle.key);
        if (it == m_impl->entries.end() || it->second.refcount == 0 ||
            it->second.token != handle.token) {
            return; // unknown, already idle, or a stale handle to a rebuilt entry
        }
        if (--it->second.refcount == 0) {
            if (const auto evicted = m_impl->cold.touch(handle.key)) {
                m_impl->evict(*evicted);
            }
        }
    }

    std::size_t resource_cache::resident_count() const noexcept {
        return m_impl->entries.size();
    }

    std::size_t resource_cache::cold_count() const noexcept {
        return m_impl->cold.size();
    }
}
