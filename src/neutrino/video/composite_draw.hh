//
// Created by igor on 22/11/2026.
//

#pragma once

#include <cstdint>

#include <neutrino/video/blend_mode.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/layer_composite_settings.hh>
#include <sdlpp/utility/geometry.hh>

#include "video/target_geometry.hh"

namespace neutrino {
    class texture;
}

// Drawing a finished layer or canvas with its composite settings (painter.md §2.7, §2.8,
// §3.5; R42, R43): the placement and the alpha rules the compositor and the copy of the
// canvases into the window share.

namespace neutrino::detail {
    /// The blend a finished layer or canvas is drawn with (painter.md §2.8): blended drawing
    /// into a transparent target leaves premultiplied colour in it, so @c blend and @c add
    /// become their premultiplied forms; the others are unchanged.
    [[nodiscard]] blend_mode premultiplied(blend_mode mode) noexcept;

    /// The blend a layer of @p mode is drawn with at @p opacity: premultiplied(), except that
    /// a @c mod layer below full opacity is drawn with @c mul, which fades its tint (SDL's mod
    /// ignores alpha, so it cannot fade).
    [[nodiscard]] blend_mode composite_blend(blend_mode mode, float opacity) noexcept;

    /// Sets on @p t what @p s says about colour: composite_blend(), colour modulation
    /// @c tint × @c opacity and alpha modulation @c opacity (opacity clamped to 0..1; it scales
    /// colour too, because SDL's alpha modulation scales only alpha in the premultiplied
    /// modes). Set at every draw: a pooled texture keeps its last owner's. False if SDL refused
    /// one of them.
    [[nodiscard]] bool apply_composite_modulation(texture& t, const layer_composite_settings& s);

    /**
     * @brief How render units land on the bound target's pixels: the target's geometry (R42)
     *        and where its logical presentation puts render space.
     *
     * A value, read once per target: the compositor keeps one for all the layers it draws
     * onto the same canvas.
     */
    struct pixel_mapping {
        /// The target, with the presentation pixel_space puts back.
        target_geometry target;
        /// The render area: where the logical presentation puts render space, in pixels; the
        /// whole target without a presentation.
        sdlpp::rect <float> area; // sdlpp-ok: internal
        /// Pixels per render unit.
        float scale_x{1.0f};
        float scale_y{1.0f};

        /// @brief The mapping of the target bound now.
        [[nodiscard]] static pixel_mapping of_bound_target();
        /// @brief A rectangle in render units, in pixels.
        [[nodiscard]] sdlpp::rect <float> to_pixels(const rect& r) const noexcept; // sdlpp-ok: internal
        /// @brief A displacement in render units, in pixels: scaled, not moved.
        [[nodiscard]] fpoint scaled(const point& d) const noexcept;
    };

    /**
     * @brief Draws in the bound target's own pixels while it lives (R42, R43).
     *
     * Turns the target's logical presentation and clip off, so a texture that mirrors the
     * target lands 1:1 on its pixels; its mapping converts the settings given in render
     * units. Puts the presentation and the target's clip back as they were when it ends,
     * however the draws end. The target must stay bound meanwhile. Not copyable.
     */
    class pixel_space {
        public:
            /// @brief With the mapping of the target bound now.
            pixel_space();
            /// @brief With @p mapping, which must be the bound target's.
            explicit pixel_space(const pixel_mapping& mapping);
            /// @brief Puts back the presentation and the target's clip. Never throws: a failure
            ///        is counted in draw_failure_count().
            ~pixel_space();

            pixel_space(const pixel_space&) = delete;
            pixel_space& operator=(const pixel_space&) = delete;

            /// @brief How render units land on the target's pixels.
            [[nodiscard]] const pixel_mapping& mapping() const noexcept;
        private:
            pixel_mapping m_mapping;
            /// The target's clip before, put back as it was: the painter's, or one set
            /// straight on the renderer.
            std::optional <rect> m_clip;
    };

    /// True when drawing a texture with @p s, in place, onto a transparent target or onto the
    /// black window, gives the texture's pixels unchanged, alpha included: opacity 1, a white
    /// tint, no offset, scale, rotation, flip, source or destination, and a blend of none or
    /// blend (add keeps the target's alpha). What
    /// lets a copy be skipped: the base layer's texture serving as the canvas (R41), the
    /// bottom scene drawing straight into the window (R44).
    [[nodiscard]] bool is_identity_composite(const layer_composite_settings& s) noexcept;

    /// ENFORCE what draw_composited() needs of @p s: no diagonal flip; a finite rotation; a
    /// finite scale that is not negative; a finite opacity. A scene's frame checks its
    /// canvas's settings with it, so a bad one fails that scene's frame, not the copy into the
    /// window after every scene.
    void validate_composite(const layer_composite_settings& s);

    /// Where draw_composited() puts a texture whose settings leave @c destination unset.
    enum class in_place : std::uint8_t {
        /// The texture mirrors the target (R42): it lands on the same pixels.
        mirrored,
        /// The texture has a size of its own: it is stretched over the render area.
        render_area,
    };

    /**
     * @brief Draws @p t into the bound target with @p s, inside @p space (R43).
     *
     * @c source is in render units for a texture that mirrors the target, in its own pixels
     * otherwise; unset, all of it, or, stretched onto a @c destination, the render area of a
     * texture that mirrors the target. The rectangle it is drawn onto is @c destination, or,
     * unset, where @p where says: its @c source part, or all of it, in place. It is then flipped, scaled and rotated
     * about the pivot, measured from that rectangle — from the render area for a texture that
     * mirrors the target drawn whole — and moved by the offset, with
     * apply_composite_modulation(). A scale of 0 draws nothing. A draw SDL refuses is counted
     * in draw_failure_count(). ENFORCE: validate_composite().
     */
    void draw_composited(texture& t, const layer_composite_settings& s, in_place where,
                         const pixel_space& space);
}
