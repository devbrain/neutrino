//
// Created by igor on 15/11/2026.
//

#pragma once

#include <cstddef>

#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/texture.hh>

namespace neutrino::detail {
    class texture_allocator;

    /**
     * @brief A layer's or a canvas's hold on a render target from the texture allocator
     *        (painter.md §3.3).
     *
     * Move-only. Destroying a handle gives its texture back to the allocator's pool. A handle is
     * in one of four states:
     *
     *   - @b empty    -- default-constructed, or moved from;
     *   - @b live     -- it holds a texture;
     *   - @b failed   -- its texture could not be made; it keeps the size it asked for, and the
     *                    next texture_allocator::resize() or device reset tries again;
     *   - @b detached -- the allocator is gone (the application quit); it holds nothing.
     *
     * Every call reaches the allocator the application publishes and checks that it holds the
     * handle's slot. Without a texture every call answers as for an empty handle, except get(),
     * which enforces. Main thread only.
     */
    class texture_handle {
        friend class texture_allocator;

        public:
            /// @brief What size() returns for an empty or detached handle.
            static constexpr dim EMPTY_DIM = {0,0};
        public:
            /// @brief An empty handle.
            texture_handle();
            /// @brief Gives a live handle's texture back to the pool and frees a failed handle's
            ///        slot. Does nothing when empty or detached.
            ~texture_handle();

            texture_handle(const texture_handle&) = delete;
            texture_handle& operator = (const texture_handle&) = delete;

            /// @brief Takes @p other's texture; @p other becomes empty.
            texture_handle(texture_handle&& other) noexcept;
            /// @brief Gives this handle's own texture back first, then takes @p other's;
            ///        @p other becomes empty.
            texture_handle& operator = (texture_handle&& other) noexcept;

            /// @brief The texture, to draw it or read it. Live handles only: ENFORCE otherwise.
            [[nodiscard]] const texture& get() const;
            /// @brief The texture, to draw into it (texture::bind()). Live handles only: ENFORCE
            ///        otherwise.
            [[nodiscard]] texture& get();
            /// @brief The texture's size. A failed handle reports the size it asked for; an
            ///        empty or detached one reports EMPTY_DIM.
            [[nodiscard]] dim size() const;
            /// @brief True while the texture holds only its clear colour, and always for a
            ///        handle without a texture. The allocator sets it (acquire, resize, resets);
            ///        only mark_drawn() clears it.
            [[nodiscard]] bool needs_redraw() const;
            /// @brief Its owner has drawn it: needs_redraw() becomes false. Called by the painter
            ///        at the end of the frame. Does nothing without a texture.
            void mark_drawn();
            /// @brief Fill the texture with the spec's clear colour. needs_redraw() is unchanged.
            ///        Does nothing without a texture.
            void clear();
            /// @brief True only when live: false when empty, failed or detached.
            explicit operator bool() const;
        private:
            /// Made by texture_allocator::acquire() for the slot at @p slot_id.
            explicit texture_handle(std::size_t slot_id);
            /// Gives the slot back, if the allocator still holds it, and leaves the handle empty.
            void release ();
        private:
            /// The allocator's slot, or texture_table::INVALID_SLOT when empty.
            std::size_t m_slot_id;
    };
}
