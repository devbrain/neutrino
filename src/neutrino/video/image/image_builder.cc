//
// Created by igor on 29/10/2026.
//

#include <neutrino/video/image/image_builder.hh>

#include <cstddef>
#include <cstdint>

#include <failsafe/enforce.hh>

namespace neutrino {
    struct image_builder::impl {
        impl(dim dimensions, std::optional <uint8_t> transparent_index)
            : m_image(*image::create_rgb(dimensions.width, dimensions.height, sdlpp::pixel_format_enum::INDEX8)) {
            if (transparent_index) {
                m_image.set_color_key(*transparent_index);
            }
        }

        impl(dim dimensions, const palette& pal, std::optional <uint8_t> transparent_index)
            : m_image(*image::create_rgb(dimensions.width, dimensions.height, sdlpp::pixel_format_enum::INDEX8)) {
            if (transparent_index) {
                m_image.set_color_key(*transparent_index);
            }
            m_image.set_palette(pal);
        }

        explicit impl(dim dimensions)
            : m_image(*image::create_rgb(dimensions.width, dimensions.height, sdlpp::pixel_format_enum::RGBA32)) {
        }

        [[nodiscard]] bool is_indexed() const {
            return m_image.format() == sdlpp::pixel_format_enum::INDEX8;
        }

        // Start of row y. A surface from SDL_CreateSurface is never RLE-encoded, so its
        // pixels are addressable without locking.
        [[nodiscard]] std::uint8_t* row_start(int y) const {
            ENFORCE(y >= 0 && static_cast <std::size_t>(y) < m_image.height(), "Row", y, "is out of range");
            return static_cast <std::uint8_t*>(m_image.get_pixels()) + static_cast <std::size_t>(y) * m_image.pitch();
        }

        void check_pixel(int x, int y) const {
            ENFORCE(x >= 0 && static_cast <std::size_t>(x) < m_image.width() &&
                y >= 0 && static_cast <std::size_t>(y) < m_image.height(),
                "Pixel (", x, ",", y, ") is out of range");
        }

        image m_image;
    };

    image_builder image_builder::indexed(dim dimensions, std::optional <uint8_t> transparent_index) {
        return image_builder{std::make_unique <impl>(dimensions, transparent_index)};
    }

    image_builder image_builder::indexed(dim dimensions, const palette& pal,
                                         std::optional <uint8_t> transparent_index) {
        return image_builder{std::make_unique <impl>(dimensions, pal, transparent_index)};
    }

    image_builder image_builder::rgba(dim dimensions) {
        return image_builder{std::make_unique <impl>(dimensions)};
    }

    void image_builder::set(int x, int y, uint8_t index) {
        ENFORCE(m_pimpl->is_indexed(), "set(x, y, index) needs an indexed image");
        m_pimpl->check_pixel(x, y);
        m_pimpl->row_start(y)[x] = index;
    }

    void image_builder::set(int x, int y, color c) {
        ENFORCE(!m_pimpl->is_indexed(), "set(x, y, color) needs an RGBA image");
        m_pimpl->check_pixel(x, y);
        // RGBA32 is a byte-order format: R, G, B, A in memory on every platform.
        std::uint8_t* px = m_pimpl->row_start(y) + static_cast <std::size_t>(x) * 4;
        px[0] = c.r;
        px[1] = c.g;
        px[2] = c.b;
        px[3] = c.a;
    }

    std::span <uint8_t> image_builder::row(int y) {
        const std::size_t bytes_per_pixel = m_pimpl->is_indexed() ? 1 : 4;
        return {m_pimpl->row_start(y), m_pimpl->m_image.width() * bytes_per_pixel};
    }

    image image_builder::build() const && {
        return std::move(m_pimpl->m_image);
    }

    image_builder::image_builder(image_builder&&) noexcept = default;
    image_builder& image_builder::operator =(image_builder&&) noexcept = default;
    image_builder::~image_builder() = default;

    image_builder::image_builder(std::unique_ptr <impl> pimpl)
        : m_pimpl(std::move(pimpl)) {
    }
}
