//
// Created by igor on 15/11/2026.
//

#include <atomic>
#include <cstddef>
#include <iterator>

#include <failsafe/enforce.hh>
#include <failsafe/logger.hh>
#include <neutrino/video/globals.hh>

#include "texture_allocator.hh"
#include "texture_table.hh"
#include "services/application_scoped.hh"
#include "services/service_access.hh"
#include "video/draw_failures.hh"

namespace neutrino::detail {
    namespace {
        // Every target has this format (painter.md §3.3), so a size alone gives its bytes.
        constexpr auto TARGET_FORMAT = pixel_format::ARGB8888;

        // Set by request_trim(), consumed by begin_frame(). Not a member: a low-memory request
        // comes from any thread, possibly while on_quit() destroys the allocator, so it must
        // not touch the allocator object.
        std::atomic <bool> g_trim_requested{false};

        // In std::size_t throughout: int arithmetic overflows above ~23000 x 23000.
        [[nodiscard]] std::size_t texture_bytes(dim size) noexcept {
            return static_cast <std::size_t>(sdlpp::bytes_per_pixel(TARGET_FORMAT))
                   * static_cast <std::size_t>(size.width)
                   * static_cast <std::size_t>(size.height);
        }
    }

    texture_allocator::texture_allocator() {
        // A request from before this application, or from the last one's shutdown, is not
        // about this pool.
        g_trim_requested.store(false, std::memory_order_relaxed);
    }

    texture_allocator::~texture_allocator() = default;

    texture_handle texture_allocator::acquire(const texture_spec& spec, bool clear) {
        texture_slot slot{spec, take_texture(spec.size), /*needs_redraw=*/true};
        if (slot.tex && clear) {
            slot.tex->fill(slot.spec.clear_color);
        }
        return texture_handle(m_table.add(std::move(slot)));
    }

    void texture_allocator::resize(texture_handle& handle, dim size) {
        if (!m_table.is_valid(handle.m_slot_id)) {
            return;                      // empty: the painter acquires it at the new size
        }
        auto& slot = m_table.get(handle.m_slot_id);
        if (slot.tex && slot.spec.size == size) {
            return;                      // same size: it keeps its contents and its flag
        }
        destroy_held(slot);              // not pooled; destroyed first, so the peak is one texture
        slot.spec.size = size;
        refill(slot);                    // a failed handle is tried again at the new size
    }

    void texture_allocator::on_targets_reset() {
        // Only the contents are lost: clear each live texture and mark it. A failed handle
        // already needs a redraw; pooled contents never matter.
        m_table.for_each([](texture_slot& slot) {
            if (slot.tex) {
                slot.tex->fill(slot.spec.clear_color);
                slot.needs_redraw = true;
            }
        });
    }

    void texture_allocator::on_device_reset() {
        // Every texture is gone: the pool first, then each handle's texture, recreated in place
        // so the painters' handles stay valid. A failed handle is tried again.
        trim();
        m_table.for_each([this](texture_slot& slot) {
            destroy_held(slot);
            refill(slot);
        });
    }

    void texture_allocator::drop_pooled(const dim& size) {
        auto [itr_beg, itr_end] = m_pool.equal_range(size);
        const auto dropped = static_cast <std::size_t>(std::distance(itr_beg, itr_end));
        if (dropped == 0) {
            return;
        }
        m_pool.erase(itr_beg, itr_end);
        m_stats.pooled -= dropped;
        m_stats.pooled_bytes -= dropped * texture_bytes(size);
        m_stats.destroyed += dropped;
    }

    void texture_allocator::trim() {
        for (const auto& entry : m_pool) {
            m_stats.pooled_bytes -= texture_bytes(entry.first);
        }
        m_stats.pooled -= m_pool.size();
        m_stats.destroyed += m_pool.size();
        m_pool.clear();
    }

    void texture_allocator::request_trim() noexcept {
        g_trim_requested.store(true, std::memory_order_release);
    }

    void texture_allocator::begin_frame() {
        if (g_trim_requested.exchange(false, std::memory_order_acq_rel)) {
            trim();
        }
    }

    const pool_stats& texture_allocator::stats() const noexcept {
        return m_stats;
    }

    std::size_t texture_allocator::pooled_count(const dim& size) const {
        return m_pool.count(size);
    }

    std::optional <texture> texture_allocator::take_texture(dim size) {
        if (size.width <= 0 || size.height <= 0) {
            LOG_ERROR("can not handle textures with invalid size", size);
            count_draw_failure();
            return std::nullopt;
        }

        auto [itr_beg, itr_end] = m_pool.equal_range(size);
        if (itr_beg == itr_end) {
            auto t = texture::create(size, texture_access::target, TARGET_FORMAT);
            if (!t) {
                LOG_ERROR("failed to create texture with size", size);
                count_draw_failure();
                return std::nullopt;
            }
            ++m_stats.created;
            ++m_stats.held;
            m_stats.held_bytes += texture_bytes(size);
            return t;
        }

        auto t = std::move(itr_beg->second);
        m_pool.erase(itr_beg);
        ++m_stats.reused;
        --m_stats.pooled;
        m_stats.pooled_bytes -= texture_bytes(size);
        ++m_stats.held;
        m_stats.held_bytes += texture_bytes(size);

        return t;
    }

    void texture_allocator::give_back(std::size_t table_idx) {
        auto& slot = m_table.get(table_idx);
        if (slot.tex) {                         // a failed slot has nothing to give back
            const auto bytes = texture_bytes(slot.spec.size);
            m_pool.emplace(slot.spec.size, std::move(*slot.tex));
            --m_stats.held;
            m_stats.held_bytes -= bytes;
            ++m_stats.pooled;
            m_stats.pooled_bytes += bytes;
        }
        m_table.remove(table_idx);
    }

    void texture_allocator::destroy_held(texture_slot& slot) {
        if (!slot.tex) {
            return;
        }
        slot.tex.reset();
        --m_stats.held;
        m_stats.held_bytes -= texture_bytes(slot.spec.size);
        ++m_stats.destroyed;
    }

    void texture_allocator::refill(texture_slot& slot) {
        ENFORCE(!slot.tex, "refill would drop a held texture without counting it");
        slot.tex = take_texture(slot.spec.size);
        if (slot.tex) {
            slot.tex->fill(slot.spec.clear_color);
        }
        slot.needs_redraw = true;
    }

    const texture_slot& texture_allocator::get(std::size_t idx) const {
        return m_table.get(idx);
    }

    texture_slot& texture_allocator::get(std::size_t idx) {
        return m_table.get(idx);
    }

    bool texture_allocator::is_valid(std::size_t idx) const noexcept {
        return m_table.is_valid(idx);
    }

    std::optional <renderer_target_scope> bind_pooled(texture_handle& handle, const texture_spec& spec,
                                                      bool wipe, const target_geometry& presentation) {
        const bool fresh = handle.empty();
        if (fresh) {
            handle = require_texture_allocator().acquire(spec, false);   // cleared below, once bound
        } else {
            handle.set_clear_color(spec.clear_color);
        }
        if (!handle) {
            return std::nullopt;               // could not be made: drawing lands below
        }
        try {
            auto scope = handle.get().bind(fresh || wipe ? std::optional <color>{spec.clear_color} : std::nullopt);
            apply_presentation(presentation);
            return scope;
        } catch (...) {
            if (fresh) {
                handle = texture_handle{};     // not cleared: it may hold a previous owner's pixels
            }
            throw;
        }
    }
}

namespace neutrino {
    detail::texture_allocator& require_texture_allocator() {
        return detail::application_scoped <detail::texture_allocator>::instance();
    }

    detail::texture_allocator* maybe_texture_allocator() noexcept {
        return detail::application_scoped <detail::texture_allocator>::maybe();
    }

    pool_stats texture_pool_stats() {
        return require_texture_allocator().stats();
    }

    void trim_texture_pool() {
        require_texture_allocator().trim();
    }
}
