//
// Created by igor on 03/06/2020.
//

#ifndef NEUTRINO_SDL_SURFACE_HH
#define NEUTRINO_SDL_SURFACE_HH

#include <string>
#include <optional>

#include <neutrino/sdl/sdl2.hh>
#include <neutrino/sdl/pixel_format.hh>
#include <neutrino/sdl/palette.hh>
#include <neutrino/sdl/object.hh>
#include <neutrino/sdl/io.hh>
#include <neutrino/sdl/call.hh>
#include <neutrino/sdl/geometry.hh>
#include <neutrino/utils/macros.hh>
#include <neutrino/utils/array_view.hh>

namespace neutrino::sdl
{
    enum class blend_mode : uint32_t
    {
        NONE = SDL_BLENDMODE_NONE,
        BLEND = SDL_BLENDMODE_BLEND,
        ADD = SDL_BLENDMODE_ADD,
        MOD = SDL_BLENDMODE_MOD
    };

    class surface : public object<SDL_Surface>
    {
    public:
        surface();
        surface(object<SDL_Surface>&& other);
        surface(surface&& other);
        surface(const object<SDL_Window>& other);
        surface& operator = (object<SDL_Surface>&& other);



        surface& operator = (const surface& other) = delete;
        surface(const surface& other) = delete;

        surface(unsigned width, unsigned height, pixel_format format);

        [[nodiscard]] static surface make_8bit(unsigned width, unsigned height);
        [[nodiscard]] static surface make_rgba_32bit(unsigned width, unsigned height);
        [[nodiscard]] static surface from_bmp(const std::string& path);
        [[nodiscard]] static surface from_bmp(io& stream);


        void save_bmp  (const std::string& path) const;
        void save_bmp  (io& stream) const;

        void lock () noexcept ;
        void unlock () noexcept ;
        [[nodiscard]] bool must_lock () const noexcept ;

        [[nodiscard]] rect clip () const;
        /**
         * @brief set clip rectangle
         * @param r
         * @return Returns true if the rectangle intersects the surface, otherwise false and blits will be completely clipped.
         */
        bool clip (const rect& r);

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
        void blit (const rect& srect, object<SDL_Surface>& dst, const rect& drect) const;

        /**
         * @brief Blit this surface to @c dst surface
         *
         * This function blits the whole surface to the destination surface @c dst.
         * Scaling will be performed if needed.
         *
         * @param dst destination surface
         * @throws @see sdl::exception on failure
         */
        void blit_scaled (const rect& srect, object<SDL_Surface>& dst) const;


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
        void blit (const rect& srect, object<SDL_Surface>& dst, const point& dpoint) const;

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
        void blit (const rect& srect, object<SDL_Surface>& dst) const;
        /**
         * @brief Blit this surface to @c dst surface
         *
         * This function blits the whole surface to the (0,0) point in the destination surface.
         * Clipping will be performed if needed.
         *
         * @param dst destination surface
         * @throws @see sdl::exception on failure
         */
        void blit (object<SDL_Surface>& dst) const;

        // returns pixels, pitch, w, h
        [[nodiscard]] std::tuple<void*, std::size_t, unsigned, unsigned> pixels_data () const;

        void fill (const rect& r, uint32_t c);
        void fill (const rect& r, const color& c);
        void fill (uint32_t c);
        void fill (const color& c);
        void fill (const utils::array_view1d<rect>& rects, uint32_t c);
        void fill (const utils::array_view1d<rect>& rects, const color& c);

        surface convert (const pixel_format& fmt) const;

        [[nodiscard]] pixel_format get_pixel_format() const;
        [[nodiscard]] uint32_t map_color(const color& c);

        // this method returns reference to the actual palette
        [[nodiscard]] palette get_palette() const;
    };
    namespace detail
    {
        template <bool LOCK_IF_NEEDED>
        class lock_surface
        {
        public:
            explicit lock_surface (surface& s)
                    : srf (s),
                      locked(true)
            {
                if constexpr (LOCK_IF_NEEDED)
                {
                    if (srf.must_lock ())
                    {
                        srf.lock ();
                    }
                }
                else
                {
                    srf.lock ();
                }
            }

            ~lock_surface () noexcept
            {
                srf.unlock();
            }

            void release ()
            {
                locked = false;
            }
            explicit operator bool () const
            {
                return locked;
            }
        private:
            surface& srf;
            bool locked;
        };
    }
}
#define LOCKED_SURFACE(S)  for(::sdl::detail::lock_surface<true> ANONYMOUS_VAR(lock_) = S; ANONYMOUS_VAR(lock_); ANONYMOUS_VAR(lock_).release())
#define LOCKED_SURFACE_ALWAYS(S)  for(::sdl::detail::lock_surface<false> ANONYMOUS_VAR(lock_) = S; ANONYMOUS_VAR(lock_); ANONYMOUS_VAR(lock_).release())
// ===================================================================================================
// Implementation
// ===================================================================================================
namespace neutrino::sdl
{

    inline
    surface::surface() = default;
    // ----------------------------------------------------------------------------------------------
    inline
    surface::surface(object<SDL_Surface>&& other)
    : object<SDL_Surface>(std::move(other))
    {

    }
    // ----------------------------------------------------------------------------------------------
    inline
    surface::surface(surface&& other)
    : object<SDL_Surface>(nullptr, false)
    {
        other.swap(*this);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    surface::surface(const object<SDL_Window>& other)
    : object<SDL_Surface>(SAFE_SDL_CALL(SDL_GetWindowSurface, const_cast<SDL_Window*>(other.handle())), false)
    {

    }
    // ----------------------------------------------------------------------------------------------
    inline
    surface& surface::operator = (object<SDL_Surface>&& other)
    {
        object<SDL_Surface>::operator=(std::move(other));
        return *this;
    }
    // ----------------------------------------------------------------------------------------------
    inline
    surface::surface(unsigned width, unsigned height, pixel_format format)
    : object<SDL_Surface>(SAFE_SDL_CALL(SDL_CreateRGBSurfaceWithFormat, 0,
            static_cast<int>(width), static_cast<int>(height), format.get_bits_per_pixels(), format.value()), true)
    {

    }
    // ----------------------------------------------------------------------------------------------
    inline
    surface surface::make_8bit(unsigned width, unsigned height)
    {
        return surface(width, height, pixel_format::make_8bit());
    }
    // ----------------------------------------------------------------------------------------------
    inline
    surface surface::make_rgba_32bit(unsigned width, unsigned height)
    {
        return surface(width, height, pixel_format::make_rgba_32bit());
    }
    // ----------------------------------------------------------------------------------------------
    inline
    surface surface::from_bmp(const std::string& path)
    {
        return surface(object<SDL_Surface >(SAFE_SDL_CALL(SDL_LoadBMP_RW, SDL_RWFromFile(path.c_str(), "rb"), 1), true));
    }
    // ----------------------------------------------------------------------------------------------
    inline
    surface surface::from_bmp(io& stream)
    {
        return surface(object<SDL_Surface >(SAFE_SDL_CALL(SDL_LoadBMP_RW, stream.handle(), 0), true));
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::save_bmp  (const std::string& path) const
    {
        SAFE_SDL_CALL(SDL_SaveBMP_RW, const_handle(), SDL_RWFromFile(path.c_str(), "wb"), 1);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::save_bmp  (io& stream) const
    {
        SAFE_SDL_CALL(SDL_SaveBMP_RW, const_handle(), stream.handle(), 0);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::lock () noexcept
    {
        SDL_LockSurface(handle());
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::unlock () noexcept
    {
        SDL_UnlockSurface(handle());
    }
    // ----------------------------------------------------------------------------------------------
    inline
    bool surface::must_lock () const noexcept
    {
        return SDL_MUSTLOCK(handle());
    }
    // ----------------------------------------------------------------------------------------------
    inline
    rect surface::clip () const
    {
        SDL_Rect r;
        SDL_GetClipRect(const_handle(), &r);
        return rect{r};
    }
    // ----------------------------------------------------------------------------------------------
    inline
    bool surface::clip (const rect& r)
    {
        return SDL_TRUE == SDL_SetClipRect(handle(), &r);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::color_key (uint32_t c)
    {
        SAFE_SDL_CALL(SDL_SetColorKey, handle(), SDL_TRUE, c);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::color_key (const color& c)
    {
        SAFE_SDL_CALL(SDL_SetColorKey, handle(), SDL_TRUE, map_color(c));
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::disable_color_key ()
    {
        SAFE_SDL_CALL(SDL_SetColorKey, handle(), SDL_FALSE, 0);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    std::optional <uint32_t> surface::color_key() const
    {
        uint32_t c;
        auto rc = SDL_GetColorKey(const_handle(), &c);
        if (0 == rc)
        {
            return c;
        }
        return std::nullopt;
    }
    // ----------------------------------------------------------------------------------------------
    inline
    blend_mode surface::blend () const
    {
        SDL_BlendMode  x;
        SAFE_SDL_CALL(SDL_GetSurfaceBlendMode,const_handle(), &x);
        return static_cast<blend_mode>(x);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::blend (blend_mode v)
    {
        SAFE_SDL_CALL(SDL_SetSurfaceBlendMode, handle(), static_cast<SDL_BlendMode>(v));
    }
    // ----------------------------------------------------------------------------------------------
    inline
    uint8_t surface::alpha_mod () const
    {
        uint8_t x;
        SAFE_SDL_CALL(SDL_GetSurfaceAlphaMod,const_handle(), &x);
        return x;
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::alpha_mod (uint8_t v)
    {
        SAFE_SDL_CALL(SDL_SetSurfaceAlphaMod, handle(), v);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::rle_acceleration (bool enabled)
    {
        SAFE_SDL_CALL(SDL_SetSurfaceRLE, handle(), enabled ? 1 : 0);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::color_mod(uint8_t r, uint8_t g, uint8_t b)
    {
        SAFE_SDL_CALL(SDL_SetSurfaceColorMod, handle(), r, g, b);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    std::optional <color> surface::color_mod () const
    {
        color c{0,0,0,0};
        if (0 == SDL_GetSurfaceColorMod(const_handle(), &c.r, &c.g, &c.b))
        {
            return c;
        }
        return std::nullopt;
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::blit (const rect& srect, object<SDL_Surface>& dst, const rect& drect) const
    {
        SAFE_SDL_CALL(SDL_BlitSurface,
                const_handle(), const_cast<rect*>(&srect),
                dst.handle(), const_cast<rect*>(&drect));
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::blit_scaled (const rect& srect, object<SDL_Surface>& dst) const
    {
        SAFE_SDL_CALL(SDL_BlitScaled, const_handle(), &srect, dst.handle(), nullptr);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::blit (const rect& srect, object<SDL_Surface>& dst, const point& dpoint) const
    {
        rect dstrect{dpoint, srect.w, srect.h};
        blit(srect, dst, dstrect);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::blit (const rect& srect, object<SDL_Surface>& dst) const
    {
        SAFE_SDL_CALL(SDL_BlitSurface,
                      const_handle(), &srect,
                      dst.handle(), nullptr);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::blit (object<SDL_Surface>& dst) const
    {
        SAFE_SDL_CALL(SDL_BlitSurface,
                      const_handle(), nullptr,
                      dst.handle(), nullptr);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    std::tuple<void*, std::size_t, unsigned, unsigned> surface::pixels_data () const
    {
        const auto* s = handle();
        return {s->pixels, s->pitch, s->w, s->h};
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::fill (const rect& r, uint32_t c)
    {
        SAFE_SDL_CALL(SDL_FillRect, handle(), &r, c);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::fill (const rect& r, const color& c)
    {
        SAFE_SDL_CALL(SDL_FillRect, handle(), &r, map_color(c));
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::fill (uint32_t c)
    {
        SAFE_SDL_CALL(SDL_FillRect, handle(), nullptr, c);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::fill (const color& c)
    {
        SAFE_SDL_CALL(SDL_FillRect, handle(), nullptr, map_color(c));
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::fill (const utils::array_view1d<rect>& rects, uint32_t c)
    {
        SAFE_SDL_CALL(SDL_FillRects, handle(), rects.begin(), rects.size(), c);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    void surface::fill (const utils::array_view1d<rect>& rects, const color& c)
    {
        SAFE_SDL_CALL(SDL_FillRects, handle(), rects.begin(), rects.size(), map_color(c));
    }
    // ----------------------------------------------------------------------------------------------
    inline
    surface surface::convert (const pixel_format& fmt) const
    {
        SDL_Surface* s = SAFE_SDL_CALL(SDL_ConvertSurfaceFormat, const_handle(), fmt.value(), 0);
        return surface{object<SDL_Surface>(s, true)};
    }
    // ----------------------------------------------------------------------------------------------
    inline
    pixel_format surface::get_pixel_format() const
    {
        return pixel_format(handle()->format->format);
    }
    // ----------------------------------------------------------------------------------------------
    inline
    uint32_t surface::map_color(const color& c)
    {
        if (get_pixel_format().is_alpha())
        {
            return SDL_MapRGBA(handle()->format, c.r, c.g, c.b, c.a);
        }
        return SDL_MapRGB(handle()->format, c.r, c.g, c.b);
    }
    // --------------------------------------------------------------------------------------------
    inline
    palette surface::get_palette() const
    {
        return palette(*this);
    }
}
#endif
