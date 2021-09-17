#ifndef NEUTRION_SDL_PIXEL_FORMAT_HH
#define NEUTRION_SDL_PIXEL_FORMAT_HH

#include <ostream>
#include <cstdint>
#include <tuple>

#include <hal/sdl/sdl2.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::sdl {
  class pixel_format {
    public:
      enum format : std::uint32_t {
        INDEX1LSB = SDL_PIXELFORMAT_INDEX1LSB,
        INDEX1MSB = SDL_PIXELFORMAT_INDEX1MSB,
        INDEX4LSB = SDL_PIXELFORMAT_INDEX4LSB,
        INDEX4MSB = SDL_PIXELFORMAT_INDEX4MSB,
        INDEX8 = SDL_PIXELFORMAT_INDEX8,
        RGB332 = SDL_PIXELFORMAT_RGB332,
        RGB444 = SDL_PIXELFORMAT_RGB444,
        RGB555 = SDL_PIXELFORMAT_RGB555,
        BGR555 = SDL_PIXELFORMAT_BGR555,
        ARGB4444 = SDL_PIXELFORMAT_ARGB4444,
        RGBA4444 = SDL_PIXELFORMAT_RGBA4444,
        ABGR4444 = SDL_PIXELFORMAT_ABGR4444,
        BGRA4444 = SDL_PIXELFORMAT_BGRA4444,
        ARGB1555 = SDL_PIXELFORMAT_ARGB1555,
        RGBA5551 = SDL_PIXELFORMAT_RGBA5551,
        ABGR1555 = SDL_PIXELFORMAT_ABGR1555,
        BGRA5551 = SDL_PIXELFORMAT_BGRA5551,
        RGB565 = SDL_PIXELFORMAT_RGB565,
        BGR565 = SDL_PIXELFORMAT_BGR565,
        RGB24 = SDL_PIXELFORMAT_RGB24,
        BGR24 = SDL_PIXELFORMAT_BGR24,
        RGB888 = SDL_PIXELFORMAT_RGB888,
        RGBX8888 = SDL_PIXELFORMAT_RGBX8888,
        BGR888 = SDL_PIXELFORMAT_BGR888,
        BGRX8888 = SDL_PIXELFORMAT_BGRX8888,
        ARGB8888 = SDL_PIXELFORMAT_ARGB8888,
        RGBA8888 = SDL_PIXELFORMAT_RGBA8888,
        ABGR8888 = SDL_PIXELFORMAT_ABGR8888,
        BGRA8888 = SDL_PIXELFORMAT_BGRA8888,
        ARGB2101010 = SDL_PIXELFORMAT_ARGB2101010,

        RGBA32 = SDL_PIXELFORMAT_RGBA32,
        ARGB32 = SDL_PIXELFORMAT_ARGB32,
        BGRA32 = SDL_PIXELFORMAT_BGRA32,
        ABGR32 = SDL_PIXELFORMAT_ABGR32,

        YV12 = SDL_PIXELFORMAT_YV12,  /**< Planar mode: Y + V + U  (3 planes) */
        IYUV = SDL_PIXELFORMAT_IYUV,  /**< Planar mode: Y + U + V  (3 planes) */
        YUY2 = SDL_PIXELFORMAT_YUY2,  /**< Packed mode: Y0+U0+Y1+V0 (1 plane) */
        UYVY = SDL_PIXELFORMAT_UYVY,  /**< Packed mode: U0+Y0+V0+Y1 (1 plane) */
        YVYU = SDL_PIXELFORMAT_YVYU,  /**< Packed mode: Y0+V0+Y1+U0 (1 plane) */
        NV12 = SDL_PIXELFORMAT_NV12,  /**< Planar mode: Y + U/V interleaved  (2 planes) */
        NV21 = SDL_PIXELFORMAT_NV21,  /**< Planar mode: Y + V/U interleaved  (2 planes) */
        OES = SDL_PIXELFORMAT_EXTERNAL_OES,       /**< Android video texture format */
      };

      enum class type : uint8_t {
          UNKNOWN = SDL_PIXELTYPE_UNKNOWN,
          INDEX1 = SDL_PIXELTYPE_INDEX1,
          INDEX4 = SDL_PIXELTYPE_INDEX4,
          INDEX8 = SDL_PIXELTYPE_INDEX8,
          PACKED8 = SDL_PIXELTYPE_PACKED8,
          PACKED16 = SDL_PIXELTYPE_PACKED16,
          PACKED32 = SDL_PIXELTYPE_PACKED32,
          ARRAYU8 = SDL_PIXELTYPE_ARRAYU8,
          ARRAYU16 = SDL_PIXELTYPE_ARRAYU16,
          ARRAYU32 = SDL_PIXELTYPE_ARRAYU32,
          ARRAYF16 = SDL_PIXELTYPE_ARRAYF16,
          ARRAYF32 = SDL_PIXELTYPE_ARRAYF32
      };
      enum class order : uint8_t {
          NONE = SDL_BITMAPORDER_NONE,
          ORDER_4321 = SDL_BITMAPORDER_4321,
          ORDER_1234 = SDL_BITMAPORDER_1234
      };
      enum class component_order : uint8_t {
          NONE = SDL_PACKEDORDER_NONE,
          XRGB = SDL_PACKEDORDER_XRGB,
          RGBX = SDL_PACKEDORDER_RGBX,
          ARGB = SDL_PACKEDORDER_ARGB,
          RGBA = SDL_PACKEDORDER_RGBA,
          XBGR = SDL_PACKEDORDER_XBGR,
          BGRX = SDL_PACKEDORDER_BGRX,
          ABGR = SDL_PACKEDORDER_ABGR,
          BGRA = SDL_PACKEDORDER_BGRA
      };
      enum class array_order : uint8_t {
          NONE = SDL_ARRAYORDER_NONE,
          RGB = SDL_ARRAYORDER_RGB,
          RGBA = SDL_ARRAYORDER_RGBA,
          ARGB = SDL_ARRAYORDER_ARGB,
          BGR = SDL_ARRAYORDER_BGR,
          BGRA = SDL_ARRAYORDER_BGRA,
          ABGR = SDL_ARRAYORDER_ABGR
      };

      enum class layout : uint8_t {
          NONE = SDL_PACKEDLAYOUT_NONE,
          LAYOUT_332 = SDL_PACKEDLAYOUT_332,
          LAYOUT_4444 = SDL_PACKEDLAYOUT_4444,
          LAYOUT_1555 = SDL_PACKEDLAYOUT_1555,
          LAYOUT_5551 = SDL_PACKEDLAYOUT_5551,
          LAYOUT_565 = SDL_PACKEDLAYOUT_565,
          LAYOUT_8888 = SDL_PACKEDLAYOUT_8888,
          LAYOUT_2101010 = SDL_PACKEDLAYOUT_2101010,
          LAYOUT_1010102 = SDL_PACKEDLAYOUT_1010102
      };
    public:
      explicit pixel_format (std::uint32_t f);
      pixel_format (format f);
      pixel_format (uint8_t bpp, uint32_t rmask, uint32_t gmask, uint32_t bmask, uint32_t amask);

      static pixel_format make_8bit ();
      static pixel_format make_rgba_32bit ();

      [[nodiscard]] explicit operator std::uint32_t () const noexcept;
      [[nodiscard]] uint32_t value () const noexcept;

      [[nodiscard]] format get_format () const noexcept;
      [[nodiscard]] type get_type () const noexcept;
      [[nodiscard]] order get_order () const noexcept;
      [[nodiscard]] component_order get_component_order () const noexcept;
      [[nodiscard]] array_order get_array_order () const noexcept;
      [[nodiscard]] layout get_layout_order () const noexcept;
      [[nodiscard]] std::uint8_t get_bits_per_pixels () const noexcept;
      [[nodiscard]] std::uint8_t get_bytes_per_pixels () const noexcept;
      [[nodiscard]] bool is_indexed () const noexcept;
      [[nodiscard]] bool is_array () const noexcept;
      [[nodiscard]] bool is_alpha () const noexcept;
      [[nodiscard]] bool is_fourcc () const noexcept;
      [[nodiscard]] bool is_packed () const noexcept;

      // bpp, rmask, gmask, bmask, amask
      [[nodiscard]] std::tuple<std::uint8_t,
                               std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t> get_mask () const;
    private:
      std::uint32_t m_value;
  };
}

inline
std::ostream& operator << (std::ostream& os, const neutrino::sdl::pixel_format& f) {
  os << SDL_GetPixelFormatName (f.value ());
  return os;
}
// ===================================================================================================
// Implementation
// ===================================================================================================
namespace neutrino::sdl {
  // ----------------------------------------------------------------------------------------------
  inline
  pixel_format pixel_format::make_8bit () {
    return pixel_format (pixel_format::INDEX8);
  }

  // ----------------------------------------------------------------------------------------------
  inline
  pixel_format pixel_format::make_rgba_32bit () {
    return pixel_format (pixel_format::RGBA32);
  }

  // ----------------------------------------------------------------------------------------------
  inline
  pixel_format::pixel_format (uint8_t bpp, uint32_t rmask, uint32_t gmask, uint32_t bmask, uint32_t amask)
      : m_value (SDL_MasksToPixelFormatEnum (static_cast<int>(bpp), rmask, gmask, bmask, amask)) {
    if (m_value == SDL_PIXELFORMAT_UNKNOWN) {
      RAISE_EX("Can not create pixel format from the provided parameters");
    }
  }

  // --------------------------------------------------------------------------------
  inline
  pixel_format::pixel_format (std::uint32_t f)
      : m_value (f) {

  }

  // --------------------------------------------------------------------------------
  inline
  pixel_format::pixel_format (format f)
      : m_value (static_cast<uint32_t>(f)) {

  }

  // --------------------------------------------------------------------------------
  inline
  pixel_format::operator std::uint32_t () const noexcept {
    return value ();
  }

  // --------------------------------------------------------------------------------
  inline
  uint32_t pixel_format::value () const noexcept {
    return m_value;
  }

  // --------------------------------------------------------------------------------
  inline
  pixel_format::format pixel_format::get_format () const noexcept {
    return static_cast<format>(m_value);
  }

  // --------------------------------------------------------------------------------
  inline
  pixel_format::type pixel_format::get_type () const noexcept {
    return static_cast<type>(SDL_PIXELFLAG(m_value));
  }

  // --------------------------------------------------------------------------------
  inline
  pixel_format::order pixel_format::get_order () const noexcept {
    return static_cast<order>(SDL_PIXELORDER(m_value));
  }

  // --------------------------------------------------------------------------------
  inline
  pixel_format::component_order pixel_format::get_component_order () const noexcept {
    return static_cast<component_order>(SDL_PIXELORDER(m_value));
  }

  // --------------------------------------------------------------------------------
  inline
  pixel_format::array_order pixel_format::get_array_order () const noexcept {
    return static_cast<array_order>(SDL_PIXELORDER(m_value));
  }

  // --------------------------------------------------------------------------------
  inline
  pixel_format::layout pixel_format::get_layout_order () const noexcept {
    return static_cast<layout>(SDL_PIXELLAYOUT(m_value));
  }

  // --------------------------------------------------------------------------------
  inline
  std::uint8_t pixel_format::get_bits_per_pixels () const noexcept {
    return SDL_BITSPERPIXEL(m_value);
  }

  // --------------------------------------------------------------------------------
  inline
  std::uint8_t pixel_format::get_bytes_per_pixels () const noexcept {
    return SDL_BYTESPERPIXEL(m_value);
  }

  // --------------------------------------------------------------------------------
  inline
  bool pixel_format::is_indexed () const noexcept {
    return SDL_ISPIXELFORMAT_INDEXED(m_value) != 0;
  }

  // --------------------------------------------------------------------------------
  inline
  bool pixel_format::is_array () const noexcept {
    return SDL_ISPIXELFORMAT_ARRAY(m_value) != 0;
  }

  // --------------------------------------------------------------------------------
  inline
  bool pixel_format::is_alpha () const noexcept {
    return SDL_ISPIXELFORMAT_ALPHA(m_value) != 0;
  }

  // --------------------------------------------------------------------------------
  inline
  bool pixel_format::is_fourcc () const noexcept {
    return SDL_ISPIXELFORMAT_FOURCC(m_value) != 0;
  }

  // --------------------------------------------------------------------------------
  inline
  bool pixel_format::is_packed () const noexcept {
    return SDL_ISPIXELFORMAT_PACKED(m_value);
  }

  // --------------------------------------------------------------------------------
  inline
  std::tuple<std::uint8_t,
             std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t> pixel_format::get_mask () const {
    int bpp;
    uint32_t r, g, b, a;
    if (SDL_TRUE == SDL_PixelFormatEnumToMasks (m_value, &bpp, &r, &g, &b, &a)) {
      return {static_cast<uint8_t>(bpp), r, g, b, a};
    }
    RAISE_EX("No pixel format conversion is possible");
  }
}

#endif
