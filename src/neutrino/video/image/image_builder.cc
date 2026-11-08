//
// Created by igor on 29/10/2026.
//

#include <neutrino/video/image/image_builder.hh>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <type_traits>
#include <utility>

#include <failsafe/enforce.hh>

namespace neutrino {
    // row() of an RGBA image views its RGBA32 bytes as colours.
    static_assert(sizeof(color) == 4 && alignof(color) == 1, "color must be 4 bytes R, G, B, A");
    static_assert(std::is_trivially_copyable_v <color>);

    namespace {
        image create_image(dim dimensions, pixel_format format) {
            auto created = image::create_rgb(dimensions.width, dimensions.height, format);
            ENFORCE(created.has_value(), "Can not create a", dimensions.width, "x", dimensions.height, "image");
            return std::move(*created);
        }

        void apply_key(image& img, std::optional <std::uint8_t> transparent_index) {
            if (transparent_index) {
                const auto set = img.set_color_key(*transparent_index);
                ENFORCE(set.has_value(), "Can not set the colour key");
            }
        }

        // Start of row y. A surface from SDL_CreateSurface is never RLE-encoded, so its
        // pixels are addressable without locking.
        std::uint8_t* row_start(const image& img, int y) {
            ENFORCE(y >= 0 && static_cast <std::size_t>(y) < img.height(), "Row", y, "is out of range");
            return static_cast <std::uint8_t*>(img.get_pixels()) + static_cast <std::size_t>(y) * img.pitch();
        }

        void check_pixel(const image& img, int x, int y) {
            ENFORCE(x >= 0 && static_cast <std::size_t>(x) < img.width() &&
                y >= 0 && static_cast <std::size_t>(y) < img.height(),
                "Pixel (", x, ",", y, ") is out of range");
        }
    }

    template<image_kind K>
    image_builder <K>::image_builder(dim dimensions, std::optional <std::uint8_t> transparent_index)
        requires (K == image_kind::indexed)
        : m_image(create_image(dimensions, pixel_format::INDEX8)) {
        apply_key(m_image, transparent_index);
    }

    template<image_kind K>
    image_builder <K>::image_builder(dim dimensions, const palette& pal, std::optional <std::uint8_t> transparent_index)
        requires (K == image_kind::indexed)
        : m_image(create_image(dimensions, pixel_format::INDEX8)) {
        apply_key(m_image, transparent_index);
        const auto set = m_image.set_palette(pal);
        ENFORCE(set.has_value(), "Can not set the palette");
    }

    template<image_kind K>
    image_builder <K>::image_builder(dim dimensions)
        requires (K == image_kind::rgba)
        : m_image(create_image(dimensions, pixel_format::RGBA32)) {
    }

    template<image_kind K>
    void image_builder <K>::set(int x, int y, pixel_type p) {
        check_pixel(m_image, x, y);
        row(y)[static_cast <std::size_t>(x)] = p;
    }

    template<image_kind K>
    void image_builder <K>::fill(pixel_type p) {
        for (int y = 0; y < static_cast <int>(m_image.height()); y++) {
            const auto r = row(y);
            std::fill(r.begin(), r.end(), p);
        }
    }

    template<image_kind K>
    std::span <typename image_builder <K>::pixel_type> image_builder <K>::row(int y) {
        // RGBA32 is a byte-order format: R, G, B, A in memory on every platform, the layout of color.
        auto* start = reinterpret_cast <pixel_type*>(row_start(m_image, y));
        return {start, m_image.width()};
    }

    template<image_kind K>
    dim image_builder <K>::size() const noexcept {
        return {static_cast <int>(m_image.width()), static_cast <int>(m_image.height())};
    }

    template<image_kind K>
    image image_builder <K>::build() && {
        return std::move(m_image);
    }

    // Exported through the extern template declarations in the header, which carry NEUTRINO_EXPORT.
    template class image_builder <image_kind::indexed>;
    template class image_builder <image_kind::rgba>;
}
