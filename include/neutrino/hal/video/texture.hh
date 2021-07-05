//
// Created by igor on 30/06/2021.
//

#ifndef NEUTRINO_HAL_VIDEO_TEXTURE_HH
#define NEUTRINO_HAL_VIDEO_TEXTURE_HH

#include <cstdint>
#include <tuple>
#include <neutrino/utils/spimpl.h>
#include <neutrino/math/rect.hh>
#include <neutrino/hal/video/pixel_format.hh>
#include <neutrino/hal/video/surface.hh>
#include <neutrino/hal/video/color.hh>

namespace neutrino::hal {
    namespace detail {
        struct texture_impl;
    }

    class renderer;

    class texture {
        friend class renderer;
    public:
        enum class access
        {
            STATIC,
            STREAMING,
            TARGET
        };
    public:
        texture();

        texture (const renderer& r, const pixel_format& format, unsigned w, unsigned h, access flags);
        texture (const renderer& r, const surface& s);

        operator bool() const;

        // returns: pixel format, texture_access, w, h
        std::tuple<pixel_format, access, unsigned, unsigned> query() const;

        [[nodiscard]] uint8_t alpha() const;
        void alpha(uint8_t a);

        [[nodiscard]] blend_mode blend() const;
        void blend(blend_mode bm);

        [[nodiscard]] std::optional<color> color_mod() const;
        void color_mod(const color& c);

        /*
        Use this function to lock  whole texture for write-only pixel access.
        returns: pointer to pixels and pitch, ie., the length of one row in bytes
        */
        [[nodiscard]] std::pair<void*, std::size_t> lock() const;
        /*
        Use this function to lock a portion of the texture for write-only pixel access.
        returns: pointer to pixels and pitch, ie., the length of one row in bytes
        */
        [[nodiscard]] std::pair<void*, std::size_t> lock(const math::rect& r) const;

        void unlock() const;

        // slow updates
        void update(const void* pixels, std::size_t pitch);
        void update(const math::rect& area, const void* pixels, std::size_t pitch);

        [[nodiscard]] uint32_t map_rgba(const color& c) const;
        [[nodiscard]] uint32_t map_rgb(const color& c) const;
    private:
        explicit texture(std::unique_ptr<detail::texture_impl>&& t);
    private:
        spimpl::unique_impl_ptr<detail::texture_impl> m_pimpl;
    };
}


#endif
