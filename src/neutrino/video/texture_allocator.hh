//
// Created by igor on 15/11/2026.
//

#pragma once

#include <optional>
#include <unordered_map>

#include <neutrino/video/texture_pool_stats.hh>
#include "video/target_geometry.hh"
#include "video/texture_handle.hh"
#include "video/texture_spec.hh"
#include "video/texture_table.hh"

namespace neutrino::detail {
    /**
     * @brief The internal owner of the painter's render targets: every layer target and every
     *        scene's canvas (painter.md §3.3).
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
     * It is application_scoped (R38), reached with require_texture_allocator() and
     * maybe_texture_allocator(). on_ready() makes it early, so it is destroyed after anything
     * made later that may hold its handles; on_quit() destroys it when it closes the
     * application scope, after the scenes and teardown() and before SDL shuts down. Main
     * thread only, except request_trim().
     */
    class texture_allocator {
        friend class texture_handle;
        public:
            texture_allocator();
            /// @brief Destroys every texture, pooled or held. By then maybe_texture_allocator()
            ///        answers null (application_scoped clears it first), so the handles still
            ///        alive find it gone and are detached.
            ~texture_allocator();

            /// @brief A handle for @p spec: a pooled texture of its size, or a new one, cleared
            ///        to its colour unless @p clear is false — for a caller that clears it when
            ///        it binds it, which saves binding it twice (bind_pooled()); its contents are
            ///        then a previous owner's until it does. The handle needs a redraw. A zero or
            ///        negative size, or a texture SDL cannot create, gives a failed handle
            ///        (logged).
            [[nodiscard]] texture_handle acquire(const texture_spec& spec, bool clear = true);
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
            /// @brief Ask for a trim at the next begin_frame(). Any thread, at any time — also
            ///        while on_quit() is destroying the allocator, or when there is none: it
            ///        only sets an atomic flag that lives outside every allocator
            ///        (SDL_EVENT_LOW_MEMORY can arrive on any thread). A request made before an
            ///        allocator exists is dropped when it is made.
            static void request_trim() noexcept;
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
            pool_stats m_stats;
            /// The textures given back, keyed by size.
            std::unordered_multimap<dim, texture> m_pool;
    };

    /**
     * @brief Binds a render target from the pool: what a canvas and a layer do at each bind
     *        (painter.md §3.4).
     *
     * Takes a texture for @p handle from the allocator if it has none, at @p spec, and
     * clears it to the spec's colour as it binds it, so it is bound once; a kept one takes the
     * spec's clear colour for its later refills, and is cleared if @p wipe. Sets
     * @p presentation on it: a pooled texture keeps its last owner's (R42). A failed handle binds nothing and is not retried here: only resize() or a device
     * reset retries it.
     */
    [[nodiscard]] std::optional <renderer_target_scope> bind_pooled(texture_handle& handle, const texture_spec& spec,
                                                                    bool wipe, const target_geometry& presentation);
}
