//
// Created by igor on 12/07/2026.
//

#include <neutrino/video/sprite/image_identity.hh>

#include <cstddef>
#include <fstream>
#include <iterator>
#include <span>
#include <system_error>
#include <vector>

#include <failsafe/exception.hh>

#include <neutrino/detail/hash.hh>

#include <sdlpp/video/surface.hh>

#include "video/sprite/surface_lock.hh"

namespace neutrino {
    namespace {
        // Content identity of a raw image byte buffer.
        content_key hash_bytes_identity(const std::vector <std::uint8_t>& data) {
            return content_hash(std::as_bytes(std::span <const std::uint8_t>(data)));
        }

        // Content identity of a decoded surface: hash the canonical RGBA8888 the packer
        // uploads (atlas_packer converts every source), so identical raw bytes under a
        // different pixel format or palette -- which render differently -- get distinct
        // keys, and a freed address cannot alias a cold-pool entry. Locks directly
        // (sdlpp's surface::lock_guard misreads SDL3's bool return; see the risks note).
        content_key hash_surface_identity(const sdlpp::surface& surf) {
            const auto canonical = surf.convert(sdlpp::pixel_format_enum::RGBA8888);
            if (!canonical) {
                return content_key{0, 0};
            }
            const details::surface_pixel_lock lock(*canonical);
            if (!lock.ready()) {
                return content_key{0, 0};
            }
            SDL_Surface* raw = canonical->get();
            if (raw == nullptr || raw->pixels == nullptr || raw->pitch <= 0 || raw->h <= 0) {
                return content_key{0, 0};
            }
            const std::size_t bytes =
                static_cast <std::size_t>(raw->pitch) * static_cast <std::size_t>(raw->h);
            const auto* p = static_cast <const std::uint8_t*>(raw->pixels);
            return content_hash(std::as_bytes(std::span <const std::uint8_t>(p, bytes)));
        }

        // Content identity of a file's bytes (read whole; callers cache the result by
        // stat key so this only runs when the file is new or has changed).
        content_key hash_file_identity(const std::filesystem::path& path) {
            std::ifstream in(path, std::ios::binary);
            if (!in) {
                THROW_RUNTIME("image_identifier: cannot open image file: ", path.string());
            }
            const std::vector <std::uint8_t> bytes(
                (std::istreambuf_iterator <char>(in)), std::istreambuf_iterator <char>());
            return content_hash(std::as_bytes(std::span <const std::uint8_t>(bytes)));
        }
    } // namespace

    content_key image_identifier::identity(const world_image& img) {
        if (const auto* mem = std::get_if <image_from_memory>(&img.source)) {
            return hash_bytes_identity(mem->bytes);
        }
        if (const auto* surf = std::get_if <image_from_surface>(&img.source)) {
            if (surf->identity) {
                const std::uint64_t len = static_cast <std::uint64_t>(img.width) * img.height;
                return content_key{*surf->identity, len};
            }
            // No producer id: hash the pixels. A pointer key would be unsafe -- a freed
            // surface's address can be reused, colliding with a cold-pool entry.
            return surf->pixels ? hash_surface_identity(*surf->pixels) : content_key{0, 0};
        }

        const std::filesystem::path& source = std::get <image_from_disk>(img.source).source;
        if (source.empty()) {
            return content_key{0, 0};
        }

        std::error_code ec;
        const auto mtime = std::filesystem::last_write_time(source, ec);
        const auto size = std::filesystem::file_size(source, ec);
        const std::string key = source.string();
        if (!ec) {
            const auto it = m_stat_cache.find(key);
            if (it != m_stat_cache.end() && it->second.mtime == mtime && it->second.size == size) {
                return it->second.content;
            }
        }

        const content_key content = hash_file_identity(source);
        if (!ec) {
            m_stat_cache[key] = stat_id{mtime, size, content};
        }
        return content;
    }

    void fold_image_identity(std::uint64_t& digest, std::uint64_t& length,
                             image_identifier& ident, const world_image& img) {
        const content_key id = ident.identity(img);
        details::hash_combine64(digest, img.source.index());
        details::hash_combine64(digest, id.hash);
        details::hash_combine64(digest, id.length);
        details::hash_combine64(digest, img.width);
        details::hash_combine64(digest, img.height);
        length += id.length;
    }
}
