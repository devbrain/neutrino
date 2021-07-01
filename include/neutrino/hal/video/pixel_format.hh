//
// Created by igor on 26/06/2021.
//

#ifndef NEUTRINO_HAL_PIXEL_FORMAT_HH
#define NEUTRINO_HAL_PIXEL_FORMAT_HH

#include <ostream>
#include <cstdint>

namespace neutrino::hal
{
    class surface;
    class texture;

    class pixel_format
    {
        friend class surface;
        friend class texture;
    public:
        enum format : uint32_t
        {
            INDEX1LSB,
            INDEX1MSB,
            INDEX4LSB,
            INDEX4MSB,
            INDEX8,
            RGB332,
            RGB444,
            RGB555,
            BGR555,
            ARGB4444,
            RGBA4444,
            ABGR4444,
            BGRA4444,
            ARGB1555,
            RGBA5551,
            ABGR1555,
            BGRA5551,
            RGB565,
            BGR565,
            RGB24,
            BGR24,
            RGB888,
            RGBX8888,
            BGR888,
            BGRX8888,
            ARGB8888,
            RGBA8888,
            ABGR8888,
            BGRA8888,
            ARGB2101010,
            RGBA32,
            ARGB32,
            BGRA32,
            ABGR32,
            YV12,
            IYUV,
            YUY2,
            UYVY,
            YVYU,
            NV12,
            NV21,
            OES
        };

        enum class type : uint8_t
        {
            UNKNOWN,
            INDEX1,
            INDEX4,
            INDEX8,
            PACKED8,
            PACKED16,
            PACKED32,
            ARRAYU8,
            ARRAYU16,
            ARRAYU32,
            ARRAYF16,
            ARRAYF32

        };
        enum class order : uint8_t
        {
            NONE,
            ORDER_4321,
            ORDER_1234
        };
        enum class component_order : uint8_t
        {
            NONE,
            XRGB,
            RGBX,
            ARGB,
            RGBA,
            XBGR,
            BGRX,
            ABGR,
            BGRA
        };
        enum class array_order : uint8_t
        {
            NONE,
            RGB,
            RGBA,
            ARGB,
            BGR,
            BGRA,
            ABGR,
        };

        enum class layout : uint8_t
        {
            NONE,
            LAYOUT_332,
            LAYOUT_4444,
            LAYOUT_1555,
            LAYOUT_5551,
            LAYOUT_565,
            LAYOUT_8888,
            LAYOUT_2101010,
            LAYOUT_1010102
        };
    public:
        struct description {
            uint8_t bpp;
            uint32_t rmask;
            uint32_t gmask;
            uint32_t bmask;
            uint32_t amask;
        };
    public:
        explicit pixel_format(format f);
        pixel_format(uint8_t bpp, uint32_t rmask, uint32_t gmask, uint32_t bmask, uint32_t amask);
        explicit pixel_format(const description& d);

        static pixel_format make_8bit();
        static pixel_format make_rgba_32bit();

        [[nodiscard]] explicit operator std::uint32_t() const noexcept;
        [[nodiscard]] uint32_t value() const noexcept;

        [[nodiscard]] format get_format() const noexcept;
        [[nodiscard]] type get_type() const noexcept;
        [[nodiscard]] order get_order() const noexcept;
        [[nodiscard]] component_order get_component_order() const noexcept;
        [[nodiscard]] array_order get_array_order() const noexcept;
        [[nodiscard]] layout get_layout_order() const noexcept;
        [[nodiscard]] std::uint8_t get_bits_per_pixels() const noexcept;
        [[nodiscard]] std::uint8_t get_bytes_per_pixels() const noexcept;
        [[nodiscard]] bool is_indexed() const noexcept;
        [[nodiscard]] bool is_array() const noexcept;
        [[nodiscard]] bool is_alpha() const noexcept;
        [[nodiscard]] bool is_fourcc() const noexcept;
        [[nodiscard]] bool is_packed() const noexcept;

        // bpp, rmask, gmask, bmask, amask
        [[nodiscard]] description get_mask() const;
    private:
        explicit pixel_format(uint32_t f);
    private:
        uint32_t m_value;
    };

    std::ostream& operator << (std::ostream& os, pixel_format::format f);
    std::ostream& operator << (std::ostream& os, pixel_format::layout f);
    std::ostream& operator << (std::ostream& os, pixel_format::array_order f);
    std::ostream& operator << (std::ostream& os, pixel_format::component_order f);
    std::ostream& operator << (std::ostream& os, pixel_format::order f);
    std::ostream& operator << (std::ostream& os, pixel_format::type f);
}

#endif
