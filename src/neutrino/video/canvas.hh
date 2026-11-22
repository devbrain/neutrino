//
// Created by igor on 21/11/2026.
//

#pragma once

#include <optional>
#include <span>

#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/layer_style.hh>
#include <neutrino/video/layer_settings.hh>
#include <neutrino/video/texture.hh>

#include "video/target_geometry.hh"
#include "video/texture_handle.hh"

namespace neutrino::detail {
    class pixel_space;

    /**
     * @brief The internal owner of one scene's canvas, part of its scene context
     *        (painter.md §2.6, §3.1).
     *
     * The canvas is the scene's result, an engine concept the game never sees (R45). In a
     * scene without layers its render() draws straight onto it; in a scene with layers the
     * compositor draws the layers onto it. The scene manager then copies it into the target
     * as it is, 1:1, with the blend its scene's flags give (draw_canvases()).
     *
     * It holds its texture from the texture allocator, its blend, its redraw mode and the
     * frame's flags. It knows the target it mirrors and the allocator, not
     * scenes. It is made with its scene and takes its texture at its first bind (R41): at the
     * pixel size of the target it mirrors, cleared to transparent (painter.md §2.8), with that
     * target's logical presentation, so render space maps onto it as onto the target (R42).
     *
     * A frame, as the scene manager runs it (painter.md §3.2):
     *
     *   1. begin_frame(): takes the target's geometry, resizes a texture it holds, and resets
     *      the frame's flags;
     *   2. bind(): while render() runs, or the compositor; an every-frame canvas is cleared
     *      at its first bind in the frame;
     *   3. note_drawn(): something was drawn on it;
     *   4. end_frame(): a canvas drawn this frame no longer needs a redraw.
     *
     * Movable, not copyable.
     */
    class canvas {
        public:
            /// @brief A canvas with no texture yet, redrawn every frame, copied with @c blend.
            canvas() = default;

            canvas(const canvas&) = delete;
            canvas& operator =(const canvas&) = delete;
            canvas(canvas&&) noexcept = default;
            canvas& operator =(canvas&&) noexcept = default;

            /// @brief The blend the canvas is copied into the target with. Its scene's flags are
            ///        the only source: the scene manager sets it before each frame — @c none
            ///        for a scene with @c hides_below, @c blend otherwise and in a transparent
            ///        window (painter.md §2.3, R45).
            void set_blend(blend_mode mode) noexcept;
            /// @brief See set_blend().
            [[nodiscard]] blend_mode blend() const noexcept;

            /// @brief @c on_demand keeps the canvas between frames (simplex's persistent
            ///        canvas); @c every_frame clears it at its first bind in each frame. Takes
            ///        effect at the next first bind.
            void set_redraw(redraw_mode mode) noexcept;
            /// @brief See set_redraw().
            [[nodiscard]] redraw_mode redraw() const noexcept;

            /**
             * @brief Starts a frame: the scene manager calls it before the scene's render().
             *
             * The canvas takes @p geometry, the target the scene is drawn into (R42). A
             * texture it holds at another size is resized (texture_allocator::resize): the old
             * one is destroyed and the new one needs a redraw; a texture that could not be made
             * is retried only at another size. A new logical presentation takes effect at the
             * next bind. The canvas becomes not bound and not drawn this frame.
             */
            void begin_frame(const target_geometry& geometry);
            /**
             * @brief Makes the canvas the render target until the returned scope ends.
             *
             * Without a texture, takes one from the texture allocator at the frame's pixel
             * size, cleared to transparent. Sets the frame's logical presentation on it. An
             * every-frame canvas is cleared to transparent at its first bind in the frame; a
             * second bind continues drawing on it. An on-demand canvas keeps its contents.
             * Returns nothing before the first begin_frame(), and when the texture could not
             * be made: drawing then lands on the target bound before.
             */
            [[nodiscard]] std::optional <renderer_target_scope> bind();
            /// @brief Something was drawn on the canvas this frame. Called by whoever draws on
            ///        it; end_frame() reads it.
            void note_drawn() noexcept;
            /// @brief Ends a frame: if something was drawn on the canvas, its texture no longer
            ///        needs a redraw. drawn() keeps its answer until the next begin_frame().
            void end_frame();
            /// @brief Clears the canvas to transparent, if it has a texture.
            void clear();
            /// @brief Gives the texture back to the pool, and stops sharing one; the next bind
            ///        takes one again. What a scene that is not drawn in a frame does with an
            ///        every-frame canvas (R41).
            void release() noexcept;
            /**
             * @brief For this frame, shows @p t — the base layer's texture — instead of a
             *        texture of its own, which it gives back (R41).
             *
             * What combining does when the base layer's texture can serve as the result: the
             * declared layers are drawn onto it, and the canvas shows it until its next
             * begin_frame(). @p t must live as long: the base layer keeps its texture, being
             * pushed every frame. Counts as drawn.
             */
            void share(texture& t) noexcept;
            /// @brief True when the canvas shows a texture it shares this frame (share()).
            [[nodiscard]] bool shared() const noexcept;
            /// @brief release() for an every-frame canvas; an on-demand one keeps its texture,
            ///        whose contents are the point. What a scene hidden under one with
            ///        @c hides_below does each frame (R41).
            void release_every_frame() noexcept;

            /// @brief True when something was drawn on the canvas since begin_frame(). The
            ///        scene manager does not copy an every-frame canvas for which it is false.
            [[nodiscard]] bool drawn() const noexcept;
            /// @brief True when the canvas has something to show: a texture, and drawn this
            ///        frame or redrawn on demand. What draw_into_target() copies, and what the
            ///        scenes above see in composition::below().
            [[nodiscard]] bool has_content() const noexcept;
            /**
             * @brief For an on-demand canvas: true when it must be drawn in full, from its first
             *        frame, or after a resize or a device reset, until the end of a frame that
             *        drew on it. Always false for an every-frame canvas: it keeps nothing.
             */
            [[nodiscard]] bool needs_redraw() const;
            /// @brief The canvas's texture, or null while it has none: before its first bind,
            ///        after release(), or a texture that could not be made.
            [[nodiscard]] const texture* get() const;
            /// @brief The texture's size; for a texture that could not be made, the size it
            ///        asked for; texture_handle::EMPTY_DIM while it has none.
            [[nodiscard]] dim size() const;

            /**
             * @brief Draws the canvas into the bound target, 1:1, inside @p px, which turned the
             *        target's logical presentation off (draw_canvases() makes one).
             *
             * As it is, 1:1, with the premultiplied form of its blend (R45). Does nothing
             * unless has_content(). A failed draw is counted in draw_failure_count().
             */
            void draw_into_target(const pixel_space& px);
        private:
            /// The blend it is copied into the target with, from its scene's flags.
            blend_mode m_blend{blend_mode::blend};
            /// Whether the canvas is kept between frames.
            redraw_mode m_redraw{redraw_mode::every_frame};
            /// The target the canvas mirrors: its pixel size and logical presentation (R42).
            target_geometry m_geometry;
            /// From the texture allocator, at the first bind.
            texture_handle m_target;
            /// begin_frame() has run: bind() has a geometry to take its texture at.
            bool m_begun{false};
            /// Bound since begin_frame(): an every-frame canvas is cleared at its first bind.
            bool m_bound{false};
            /// Drawn on since begin_frame().
            bool m_drawn{false};
            /// The texture shown this frame instead of its own: the base layer's (R41).
            texture* m_shared{nullptr};
    };

    /**
     * @brief Draws @p canvases into the bound target in order, the first at the bottom, each
     *        with canvas::draw_into_target() (painter.md §3.2).
     *
     * The target's logical presentation and clip are off while they are drawn, so each canvas,
     * which mirrors the target (R42), lands 1:1 on its pixels; both are put back afterwards,
     * the clip as the painter's. Null entries are skipped.
     */
    void draw_canvases(std::span <canvas* const> canvases);
}
