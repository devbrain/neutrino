//
// Created by igor on 15/11/2026.
//

#pragma once

#include <optional>
#include <unordered_map>
#include <atomic>

#include <neutrino/video/texture_pool_stats.hh>
#include "video/texture_handle.hh"
#include "video/texture_spec.hh"
#include "video/texture_table.hh"

namespace neutrino::detail {
    /**
     * @brief The internal owner of the painter's render targets: every layer target and every
     *        canvas, of scene painters and of explicit painters (painter.md §3.3).
     *
     * acquire() hands out a texture_handle holding a pooled texture of the spec's size, or a new
     * one, always cleared to the spec's colour. Destroying the handle gives the texture back to
     * the pool, where acquire() and resize() reuse it by size. The allocator also keeps each
     * handle's needs_redraw() flag true to what its texture holds, through resizes, resets and
     * low memory.
     *
     * Textures made with the public texture::create, from_image or adopt are not the
     * allocator's. Every texture, the allocator's included, is made on the application's
     * renderer.
     *
     * The application owns the allocator: it is created in on_ready(), published in the service
     * locator, and in on_quit() unpublished and destroyed before SDL shuts down. Main thread
     * only, except request_trim().
     */
    class texture_allocator {
        friend class texture_handle;
        public:
            texture_allocator();
            /// @brief Destroys every texture, pooled or held. Unpublish the allocator first
            ///        (service_locator::clear_texture_allocator()): the handles still alive then
            ///        find it gone and are detached.
            ~texture_allocator();

            /// @brief A handle for @p spec: a pooled texture of its size, or a new one, cleared
            ///        to its colour; the handle needs a redraw. A zero or negative size, or a
            ///        texture SDL cannot create, gives a failed handle (logged).
            [[nodiscard]] texture_handle acquire(const texture_spec& spec);
            /// @brief Give @p handle a texture of @p size. At the same size nothing changes. At
            ///        another the old texture is destroyed, not pooled, before a new one is taken
            ///        as acquire() takes it, cleared, needing a redraw. A failed handle is tried
            ///        again; an empty one is left alone.
            void resize(texture_handle& handle, dim size);

            /// @brief Destroy the pooled textures of @p size: the old render size, after the
            ///        render size changes.
            void drop_pooled(const dim& size);
            /// @brief Destroy every pooled texture. Held textures are untouched.
            void trim();
            /// @brief Ask for a trim at the next begin_frame(). Any thread: it only sets an
            ///        atomic flag (SDL_EVENT_LOW_MEMORY can arrive on any thread).
            void request_trim();
            /// @brief The start of a frame (application::on_render): runs a requested trim.
            void begin_frame();

            /// @brief The textures held and pooled and their bytes (four a pixel); the textures
            ///        created, reused and destroyed since the allocator was made.
            [[nodiscard]] const pool_stats& stats() const noexcept;
            /// @brief The pooled textures of @p size (the tests' invariants need it per size).
            [[nodiscard]] std::size_t pooled_count(const dim& size) const;

            /// @brief SDL_EVENT_RENDER_TARGETS_RESET: clear every live handle's texture and mark
            ///        it for redraw. Failed handles already need one; the pool is left alone.
            void on_targets_reset();
            /// @brief SDL_EVENT_RENDER_DEVICE_RESET: destroy the pool, then recreate every
            ///        handle's texture in place, cleared and needing a redraw, so the handles
            ///        stay valid. A failed handle is tried again.
            void on_device_reset();
        private:
            /// A pooled texture of @p size, or a new one; nullopt (logged) if none can be made.
            /// Counted as held.
            std::optional<texture> take_texture(dim size);
            /// A dying handle's slot: its texture to the pool, then the slot freed.
            void give_back (std::size_t table_idx);
            /// Destroys a slot's texture, if it has one, and counts it; the slot stays.
            void destroy_held(texture_slot& slot);
            /// Takes a texture at the slot's size into a slot without one, clears it and marks
            /// the slot for redraw.
            void refill(texture_slot& slot);

            /// For texture_handle: the slot at @p idx. ENFORCE: occupied.
            [[nodiscard]] const texture_slot& get(std::size_t idx) const;
            /// For texture_handle: the slot at @p idx. ENFORCE: occupied.
            [[nodiscard]] texture_slot& get(std::size_t idx);
            /// For texture_handle: true if the allocator holds the slot at @p idx.
            [[nodiscard]] bool is_valid(std::size_t idx) const noexcept;
        private:
            /// The slots of the live and failed handles.
            texture_table m_table;
            /// Set by request_trim(), consumed by begin_frame().
            std::atomic<bool> m_trim_requested;
            pool_stats m_stats;
            /// The textures given back, keyed by size.
            std::unordered_multimap<dim, texture> m_pool;
    };
}
