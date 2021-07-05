//
// Created by igor on 26/06/2021.
//

#ifndef NEUTRINO_HAL_SURFACE_HH
#define NEUTRINO_HAL_SURFACE_HH

#include <optional>
#include <tuple>

#include <neutrino/utils/spimpl.h>
#include <neutrino/hal/video/pixel_format.hh>
#include <neutrino/hal/video/blend_mode.hh>
#include <neutrino/hal/video/color.hh>
#include <neutrino/math/point.hh>
#include <neutrino/math/rect.hh>


namespace neutrino::hal {

    namespace detail {
        struct surface_impl;
    }

    class window;
    class renderer;
    class image_loader;
    class texture;

    class surface final {
        friend class image_loader;
        friend class renderer;
        friend class texture;
    public:
        surface() = default;

        surface(const window& w);
        surface(unsigned width, unsigned height, pixel_format format);

        static surface make_rgba(unsigned width, unsigned height);
        static surface make_8bit(unsigned width, unsigned height);

        pixel_format format() const;
        operator bool() const;

        void lock () noexcept ;
        void unlock () noexcept ;
        [[nodiscard]] bool must_lock () const noexcept ;

        [[nodiscard]] math::rect clip () const;
        /**
         * @brief set clip rectangle
         * @param r
         * @return Returns true if the rectangle intersects the surface, otherwise false and blits will be completely clipped.
         */
        bool clip (const math::rect& r);

        void color_key (uint32_t c);
        void color_key (const color& c);
        void disable_color_key ();
        [[nodiscard]] std::optional <uint32_t> color_key() const;

        [[nodiscard]] blend_mode blend () const;
        void blend (blend_mode v);

        [[nodiscard]] uint8_t alpha_mod () const;
        void alpha_mod (uint8_t v);

        void rle_acceleration (bool enabled);

        void color_mod(uint8_t r, uint8_t g, uint8_t b);
        [[nodiscard]] std::optional <color> color_mod () const;

        /**
         * @brief Blit this surface to @c dst surface
         *
         * This function blits an area enclosed by @c srect to the destination surface area @c drect.
         * Scaling will be performed if needed.
         *
         * @param srect source area
         * @param dst  destination surface
         * @param drect destination area
         * @throws @see sdl::exception on failure
         */
        void blit (const math::rect& srect, surface& dst, const math::rect& drect) const;

        /**
         * @brief Blit this surface to @c dst surface
         *
         * This function blits the whole surface to the destination surface @c dst.
         * Scaling will be performed if needed.
         *
         * @param dst destination surface
         * @throws @see sdl::exception on failure
         */
        void blit_scaled (const math::rect& srect, surface& dst) const;
        void blit_scaled (surface& dst) const;
        void blit_scaled (const math::rect& srect, surface& dst, const math::rect& drect) const;

        /**
         * @brief Blit this surface to @c dst surface
         *
         * This function blits the source area enclosed be @srect to the point @c dpoint in the destination surface.
         * Clipping will be performed if needed.
         *
         * @param srect source area
         * @param dst  destination surface
         * @param dpoint destination point
         * @throws @see sdl::exception on failure
         */
        void blit (const math::rect& srect, surface& dst, const math::point2d& dpoint) const;

        /**
         * @brief Blit this surface to @c dst surface
         *
         * This function blits the source area enclosed be @srect to the (0,0) point in the destination surface.
         * Clipping will be performed if needed.
         *
         * @param srect source area
         * @param dst destination surface
         * @throws @see sdl::exception on failure
         */
        void blit (const math::rect& srect, surface& dst) const;
        /**
         * @brief Blit this surface to @c dst surface
         *
         * This function blits the whole surface to the (0,0) point in the destination surface.
         * Clipping will be performed if needed.
         *
         * @param dst destination surface
         * @throws @see sdl::exception on failure
         */
        void blit (surface& dst) const;

        // returns pixels, pitch, w, h
        [[nodiscard]] std::tuple<void*, std::size_t, unsigned, unsigned> pixels_data () const;

        void fill (const math::rect& r, uint32_t c);
        void fill (const math::rect& r, const color& c);
        void fill (uint32_t c);
        void fill (const color& c);


        surface convert (const pixel_format& fmt) const;

        [[nodiscard]] pixel_format get_pixel_format() const;
        [[nodiscard]] uint32_t map_color(const color& c);


        surface roto_zoom(double angle, double zoomx, double zoomy, bool smooth);

    private:
        surface(std::unique_ptr<detail::surface_impl>&& impl);
    private:
        spimpl::unique_impl_ptr<detail::surface_impl> m_pimpl;
    };
}

#endif
