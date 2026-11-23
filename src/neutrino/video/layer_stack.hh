//
// Created by igor on 16/11/2026.
//

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <neutrino/video/layer_id.hh>
#include <neutrino/video/layer_settings.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/texture.hh>

#include "video/composite_draw.hh"
#include "video/painter_context.hh"
#include "video/target_geometry.hh"
#include "video/texture_handle.hh"

namespace neutrino::detail {
    /**
     * @brief The internal owner of one scene's layers, part of its scene context
     *        (painter.md §3.4).
     *
     * It holds their settings, their targets from the texture allocator and the frame's
     * flags. It knows its size and the allocator, not scenes. The first declare() also makes
     * the base layer (R31), the bottom one, which begin_frame() pushes under every other push
     * and end_frame() pops. It keeps its own record of the pushes: a layer push changes
     * where draws go, not the painter state, so it is independent of push_state() (R39).
     *
     * In a stack with no layers declared every frame hook does nothing.
     *
     * Not copyable: the layer_ids it hands out carry its serial.
     */
    class layer_stack {
        struct entry {
            // one per layer: the base layer first
            layer_settings settings; // size, redraw mode, style (live; read when combining)
            texture_handle target; // from the texture allocator; taken at the first push
            bool pushed{false}; // pushed this frame
        };

        public:
            /// @brief A stack whose layers without a size of their own mirror @p geometry
            ///        (R42), with a serial no other stack has. A plain size reads as a target
            ///        of that many pixels without a logical presentation.
            explicit layer_stack(target_geometry geometry);
            /// @brief Ends the bindings of the layers still pushed, newest first, so the target
            ///        bound before the first push is bound again; then gives every target back.
            ~layer_stack();

            layer_stack(const layer_stack&) = delete;
            layer_stack& operator =(const layer_stack&) = delete;

            /// @brief Adds a layer after the others; takes no texture yet. The first call also
            ///        makes the base layer, before the declared one: at the stack's geometry,
            ///        redrawn every frame, with the default style. ENFORCE: not
            ///        combining.
            [[nodiscard]] layer_id declare(const layer_settings& = {});
            /// @brief The base layer: the bottom one, where drawing goes with no other layer
            ///        pushed. ENFORCE: a layer has been declared.
            [[nodiscard]] layer_id base_layer() const;

            /**
             * @brief Makes @p lid the current layer.
             *
             * Takes a target from the texture allocator if the layer has none — at its own
             * size, or the stack's geometry — cleared to its clear colour: opaque white for a
             * @c mod layer, transparent for any other (painter.md §2.8). Binds it, clearing it
             * again if this is the layer's first push in the frame and its redraw mode is
             * @c every_frame, and sets its logical presentation: the stack's for a layer
             * without a size of its own, so render space maps onto it as onto the target the
             * scene is drawn into (R42); none for a layer with one, which is drawn in its own
             * pixels. A layer whose texture could not be made is pushed with nothing
             * bound, and is not retried here: drawing into it does nothing (painter.md §3.6),
             * and the target below it is left as it was. Pushing a layer again in the same frame, or while
             * it is already pushed, continues drawing on it. The painter state is not touched:
             * a layer is where draws go, the state how (R39); the painter's clip follows onto
             * the layer. ENFORCE: @p lid is a layer of this stack; not combining — the layers
             * are finished.
             */
            void push(layer_id lid);
            /**
             * @brief Ends the most recent push: binds again the target bound before it. The
             *        painter state is not touched (R39).
             *
             * ENFORCE: something is pushed — begin_frame()'s push of the base layer does not
             * count, only end_frame() ends it — and the bound target is still the layer's
             * texture, or for a failed layer still discards: a texture::bind() scope that
             * outlived the layer fails here instead of leaving the wrong target bound.
             */
            void pop();

            /**
             * @brief Starts a frame: the scene manager calls it before the scene's render().
             *
             * The stack takes @p geometry: if its size is not the stack's, the layers without a
             * size of their own are resized to it (texture_allocator::resize); a new logical
             * presentation takes effect at their next bind. Every layer becomes not pushed this
             * frame, and the base layer is pushed, cleared, under everything else. ENFORCE: no
             * frame is open.
             */
            void begin_frame(const target_geometry& geometry);
            /**
             * @brief Ends a frame: pops the base layer, then gives back the target of every
             *        every-frame layer that was not pushed this frame. An on-demand layer keeps
             *        its target; so does a failed one, which only resize() or a device reset
             *        retries.
             *
             * ENFORCE: only the base layer is left pushed. A frame that ends with another layer
             * pushed fails, naming it, after ending every push as abort_frame() does.
             */
            void end_frame();
            /// @brief Ends every push, newest first, so the target bound before the frame is
            ///        bound again: what the scene manager calls when render() throws. Checks
            ///        nothing; never throws.
            void abort_frame() noexcept;
            /// @brief Gives back the texture of every every-frame layer, the base layer's too:
            ///        what a scene hidden under one with @c hides_below does each frame (R41).
            ///        An on-demand layer keeps its texture, and so does a failed one. Each takes
            ///        one again at its next push. ENFORCE: no frame is open.
            void release_every_frame();

            /// @brief The layer's texture, or null while it has none (never pushed, given
            ///        back, or failed). ENFORCE: @p lid is a layer of this stack.
            [[nodiscard]] const texture* target(layer_id lid) const;
            /// @brief For an on-demand layer: true when it must be drawn in full — never drawn,
            ///        or its contents lost to a resize, a change of presentation or a reset —
            ///        until the end of a frame that pushed it. Always false for an every-frame
            ///        layer. ENFORCE: @p lid is a layer of this stack.
            [[nodiscard]] bool needs_redraw(layer_id lid) const;
            /// @brief Clears the current layer — the newest push, or the base layer — to its
            ///        clear colour. A layer whose texture could not be made has nothing to
            ///        clear. ENFORCE: a frame is open.
            void clear_current();

            // ---- combining (painter.md §3.4, R43) ----------------------------------------

            /// @brief The base layer, then the declared layers, in declaration order; empty
            ///        while none is declared.
            [[nodiscard]] std::span <const layer_id> layers() const noexcept;
            /// @brief The layer's style (live): how it is drawn when combined. ENFORCE: a layer
            ///        of this stack.
            [[nodiscard]] layer_style& style(layer_id lid);
            /// @brief See style(layer_id).
            [[nodiscard]] const layer_style& style(layer_id lid) const;
            /// @brief True if the layer has something to show: a texture, and pushed this
            ///        frame or redrawn on demand without needing a redraw. One answer for the
            ///        whole frame: the layers are marked drawn only after combining. ENFORCE:
            ///        a layer of this stack.
            [[nodiscard]] bool has_content(layer_id lid) const;
            /**
             * @brief Draws the layer onto the bound target, the canvas, with @p settings
             *        (draw_composited()): in the canvas's pixels, the presentation and clip off
             *        meanwhile (R43). A layer without a size of its own mirrors the canvas; one
             *        with a size of its own is stretched over the render area unless
             *        @c destination says where. Does nothing for a layer without content.
             *        ENFORCE: a layer of this stack; combining; @p settings valid.
             */
            void draw_layer(layer_id lid, const layer_style& settings);
            /// @brief draw_layer() with its own settings for each layer from @p first to @p last,
            ///        both included. ENFORCE: both are layers of this stack, @p first not after
            ///        @p last; combining.
            void draw_layers(layer_id first, layer_id last);

            /// @brief Starts combining: pushing and declaring fail until end_combine().
            ///        ENFORCE: no frame is open, and not combining already.
            void begin_combine();
            /// @brief Ends combining. Never throws.
            void end_combine() noexcept;
            /// @brief True between begin_combine() and end_combine().
            [[nodiscard]] bool combining() const noexcept;
            /// @brief The base layer's texture, or null while it has none or it failed. For the
            ///        canvas to share (R41).
            [[nodiscard]] texture* base_texture();
            /// @brief Binds the base layer's texture, to combine the declared layers onto it
            ///        (R41): not cleared — it holds what render() drew — and with the stack's
            ///        presentation. Nothing for a base layer without a texture. ENFORCE:
            ///        combining.
            [[nodiscard]] std::optional <renderer_target_scope> bind_base();
            /// @brief The default compositor without the base layer, onto the bound target:
            ///        each declared layer with content, with its own settings, in declaration
            ///        order. ENFORCE: combining.
            void draw_declared();
            /// @brief The default compositor, onto the bound target, with the presentation and
            ///        clip turned off once for all the layers. ENFORCE: combining.
            void draw_all();
            /// @brief Marks each layer pushed this frame drawn (texture_handle::mark_drawn()),
            ///        so an on-demand one no longer needs a redraw: after combining.
            void mark_drawn();
        private:
            /// True if @p lid names a layer (it is not a default-constructed id), was made by
            /// this stack, and is one of its layers.
            [[nodiscard]] bool is_valid(const layer_id& lid) const noexcept;
            /// The next serial of a global counter, safe from any thread. A counter that wraps
            /// skips layer_id's INVALID, the value that names no stack.
            [[nodiscard]] static std::uint32_t next_serial() noexcept;
            /// The push() of the layer at @p idx; @p frame marks begin_frame()'s own push.
            void push_index(std::size_t idx, bool frame);
            /// Ends the newest push: unbinds it.
            void end_newest();
            /// has_content() of the layer at @p idx.
            [[nodiscard]] bool has_content_index(std::size_t idx) const;
            /// draw_layer() of the layer at @p idx.
            void draw_index(std::size_t idx, const layer_style& settings);
            /// Each layer with content from @p first on, with its own settings, in one
            /// pixel_space.
            void draw_from(std::size_t first);
            /// The mapping of the target bound now, read once per target while combining.
            [[nodiscard]] const pixel_mapping& bound_mapping();

        private:
            /// What a layer without a size of its own mirrors: the target's size and its
            /// logical presentation (R42). Of no size until the first begin_frame(), when a
            /// layer pushed takes the bound target's.
            target_geometry m_geometry;
            /// Carried by every layer_id this stack makes.
            std::uint32_t m_serial;

            struct pushed_entry {
                std::size_t                          layer;   // its index in m_layers
                std::optional <renderer_target_scope> target; // from texture::bind(); ending it binds the previous target; none for a failed layer
                std::optional <discard_scope>        discard; // a failed layer's instead: drawing into it does nothing
                bool                                 frame{false}; // begin_frame()'s push of the base layer
            };
            std::vector<entry> m_layers;           // the base layer, then declaration order (§3.1)
            std::vector<layer_id> m_ids;           // the ids of m_layers, for layers()
            std::vector<pushed_entry>  m_stack;    // back() is the current layer
            bool m_combining{false};               // between begin_combine() and end_combine()
            /// While combining: how render units land on the bound target, read at the first
            /// draw_layer() onto it, and the target_switches() count it was read at.
            std::optional <pixel_mapping> m_mapping;
            std::uint64_t m_mapping_switches{0};
    };
}
