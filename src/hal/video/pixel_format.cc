//
// Created by igor on 26/06/2021.
//

#include <neutrino/hal/video/pixel_format.hh>
#include <hal/sdl/sdl2.hh>
#include <neutrino/utils/exception.hh>

static neutrino::hal::pixel_format::format from_format (uint32_t t) {
  switch (t) {
    case SDL_PIXELFORMAT_INDEX1LSB :
      return neutrino::hal::pixel_format::INDEX1LSB;
    case SDL_PIXELFORMAT_INDEX1MSB :
      return neutrino::hal::pixel_format::INDEX1MSB;
    case SDL_PIXELFORMAT_INDEX4LSB :
      return neutrino::hal::pixel_format::INDEX4LSB;
    case SDL_PIXELFORMAT_INDEX4MSB :
      return neutrino::hal::pixel_format::INDEX4MSB;
    case SDL_PIXELFORMAT_INDEX8 :
      return neutrino::hal::pixel_format::INDEX8;
    case SDL_PIXELFORMAT_RGB332 :
      return neutrino::hal::pixel_format::RGB332;
    case SDL_PIXELFORMAT_RGB444 :
      return neutrino::hal::pixel_format::RGB444;
    case SDL_PIXELFORMAT_RGB555 :
      return neutrino::hal::pixel_format::RGB555;
    case SDL_PIXELFORMAT_BGR555 :
      return neutrino::hal::pixel_format::BGR555;
    case SDL_PIXELFORMAT_ARGB4444 :
      return neutrino::hal::pixel_format::ARGB4444;
    case SDL_PIXELFORMAT_RGBA4444 :
      return neutrino::hal::pixel_format::RGBA4444;
    case SDL_PIXELFORMAT_ABGR4444 :
      return neutrino::hal::pixel_format::ABGR4444;
    case SDL_PIXELFORMAT_BGRA4444 :
      return neutrino::hal::pixel_format::BGRA4444;
    case SDL_PIXELFORMAT_ARGB1555 :
      return neutrino::hal::pixel_format::ARGB1555;
    case SDL_PIXELFORMAT_RGBA5551 :
      return neutrino::hal::pixel_format::RGBA5551;
    case SDL_PIXELFORMAT_ABGR1555 :
      return neutrino::hal::pixel_format::ABGR1555;
    case SDL_PIXELFORMAT_BGRA5551 :
      return neutrino::hal::pixel_format::BGRA5551;
    case SDL_PIXELFORMAT_RGB565 :
      return neutrino::hal::pixel_format::RGB565;
    case SDL_PIXELFORMAT_BGR565 :
      return neutrino::hal::pixel_format::BGR565;
    case SDL_PIXELFORMAT_RGB24 :
      return neutrino::hal::pixel_format::RGB24;
    case SDL_PIXELFORMAT_BGR24 :
      return neutrino::hal::pixel_format::BGR24;
    case SDL_PIXELFORMAT_RGB888 :
      return neutrino::hal::pixel_format::RGB888;
    case SDL_PIXELFORMAT_RGBX8888 :
      return neutrino::hal::pixel_format::RGBX8888;
    case SDL_PIXELFORMAT_BGR888 :
      return neutrino::hal::pixel_format::BGR888;
    case SDL_PIXELFORMAT_BGRX8888 :
      return neutrino::hal::pixel_format::BGRX8888;
    case SDL_PIXELFORMAT_ARGB8888 :
      return neutrino::hal::pixel_format::ARGB8888;
    case SDL_PIXELFORMAT_RGBA8888 :
      return neutrino::hal::pixel_format::RGBA8888;
    case SDL_PIXELFORMAT_ABGR8888 :
      return neutrino::hal::pixel_format::ABGR8888;
    case SDL_PIXELFORMAT_BGRA8888 :
      return neutrino::hal::pixel_format::BGRA8888;
    case SDL_PIXELFORMAT_ARGB2101010 :
      return neutrino::hal::pixel_format::ARGB2101010;
//case SDL_PIXELFORMAT_RGBA32 :return neutrino::hal::pixel_format::RGBA32;
//case SDL_PIXELFORMAT_ARGB32 :return neutrino::hal::pixel_format::ARGB32;
//case SDL_PIXELFORMAT_BGRA32 :return neutrino::hal::pixel_format::BGRA32;
//case SDL_PIXELFORMAT_ABGR32 :return neutrino::hal::pixel_format::ABGR32;
    case SDL_PIXELFORMAT_YV12 :
      return neutrino::hal::pixel_format::YV12;
    case SDL_PIXELFORMAT_IYUV :
      return neutrino::hal::pixel_format::IYUV;
    case SDL_PIXELFORMAT_YUY2 :
      return neutrino::hal::pixel_format::YUY2;
    case SDL_PIXELFORMAT_UYVY :
      return neutrino::hal::pixel_format::UYVY;
    case SDL_PIXELFORMAT_YVYU :
      return neutrino::hal::pixel_format::YVYU;
    case SDL_PIXELFORMAT_NV12 :
      return neutrino::hal::pixel_format::NV12;
    case SDL_PIXELFORMAT_NV21 :
      return neutrino::hal::pixel_format::NV21;
    case SDL_PIXELFORMAT_EXTERNAL_OES :
      return neutrino::hal::pixel_format::OES;
    default:
      RAISE_EX("Should not be here");
  }
}

static const char *to_string (neutrino::hal::pixel_format::format f) {
  switch (f) {
    case neutrino::hal::pixel_format::INDEX1LSB:
      return "INDEX1LSB";
    case neutrino::hal::pixel_format::INDEX1MSB:
      return "INDEX1MSB";
    case neutrino::hal::pixel_format::INDEX4LSB:
      return "INDEX4LSB";
    case neutrino::hal::pixel_format::INDEX4MSB:
      return "INDEX4MSB";
    case neutrino::hal::pixel_format::INDEX8:
      return "INDEX8";
    case neutrino::hal::pixel_format::RGB332:
      return "RGB332";
    case neutrino::hal::pixel_format::RGB444:
      return "RGB444";
    case neutrino::hal::pixel_format::RGB555:
      return "RGB555";
    case neutrino::hal::pixel_format::BGR555:
      return "BGR555";
    case neutrino::hal::pixel_format::ARGB4444:
      return "ARGB4444";
    case neutrino::hal::pixel_format::RGBA4444:
      return "RGBA4444";
    case neutrino::hal::pixel_format::ABGR4444:
      return "ABGR4444";
    case neutrino::hal::pixel_format::BGRA4444:
      return "BGRA4444";
    case neutrino::hal::pixel_format::ARGB1555:
      return "ARGB1555";
    case neutrino::hal::pixel_format::RGBA5551:
      return "RGBA5551";
    case neutrino::hal::pixel_format::ABGR1555:
      return "ABGR1555";
    case neutrino::hal::pixel_format::BGRA5551:
      return "BGRA5551";
    case neutrino::hal::pixel_format::RGB565:
      return "RGB565";
    case neutrino::hal::pixel_format::BGR565:
      return "BGR565";
    case neutrino::hal::pixel_format::RGB24:
      return "RGB24";
    case neutrino::hal::pixel_format::BGR24:
      return "BGR24";
    case neutrino::hal::pixel_format::RGB888:
      return "RGB888";
    case neutrino::hal::pixel_format::RGBX8888:
      return "RGBX8888";
    case neutrino::hal::pixel_format::BGR888:
      return "BGR888";
    case neutrino::hal::pixel_format::BGRX8888:
      return "BGRX8888";
    case neutrino::hal::pixel_format::ARGB8888:
      return "ARGB8888";
    case neutrino::hal::pixel_format::RGBA8888:
      return "RGBA8888";
    case neutrino::hal::pixel_format::ABGR8888:
      return "ABGR8888";
    case neutrino::hal::pixel_format::BGRA8888:
      return "BGRA8888";
    case neutrino::hal::pixel_format::ARGB2101010:
      return "ARGB2101010";
    case neutrino::hal::pixel_format::RGBA32:
      return "RGBA32";
    case neutrino::hal::pixel_format::ARGB32:
      return "ARGB32";
    case neutrino::hal::pixel_format::BGRA32:
      return "BGRA32";
    case neutrino::hal::pixel_format::ABGR32:
      return "ABGR32";
    case neutrino::hal::pixel_format::YV12:
      return "YV12";
    case neutrino::hal::pixel_format::IYUV:
      return "IYUV";
    case neutrino::hal::pixel_format::YUY2:
      return "YUY2";
    case neutrino::hal::pixel_format::UYVY:
      return "UYVY";
    case neutrino::hal::pixel_format::YVYU:
      return "YVYU";
    case neutrino::hal::pixel_format::NV12:
      return "NV12";
    case neutrino::hal::pixel_format::NV21:
      return "NV21";
    case neutrino::hal::pixel_format::OES:
      return "OES";
    default:
      RAISE_EX("Should not be here");
  }
}

static SDL_PixelFormatEnum map_format (neutrino::hal::pixel_format::format f) {
  switch (f) {
    case neutrino::hal::pixel_format::INDEX1LSB:
      return SDL_PIXELFORMAT_INDEX1LSB;
    case neutrino::hal::pixel_format::INDEX1MSB:
      return SDL_PIXELFORMAT_INDEX1MSB;
    case neutrino::hal::pixel_format::INDEX4LSB:
      return SDL_PIXELFORMAT_INDEX4LSB;
    case neutrino::hal::pixel_format::INDEX4MSB:
      return SDL_PIXELFORMAT_INDEX4MSB;
    case neutrino::hal::pixel_format::INDEX8:
      return SDL_PIXELFORMAT_INDEX8;
    case neutrino::hal::pixel_format::RGB332:
      return SDL_PIXELFORMAT_RGB332;
    case neutrino::hal::pixel_format::RGB444:
      return SDL_PIXELFORMAT_RGB444;
    case neutrino::hal::pixel_format::RGB555:
      return SDL_PIXELFORMAT_RGB555;
    case neutrino::hal::pixel_format::BGR555:
      return SDL_PIXELFORMAT_BGR555;
    case neutrino::hal::pixel_format::ARGB4444:
      return SDL_PIXELFORMAT_ARGB4444;
    case neutrino::hal::pixel_format::RGBA4444:
      return SDL_PIXELFORMAT_RGBA4444;
    case neutrino::hal::pixel_format::ABGR4444:
      return SDL_PIXELFORMAT_ABGR4444;
    case neutrino::hal::pixel_format::BGRA4444:
      return SDL_PIXELFORMAT_BGRA4444;
    case neutrino::hal::pixel_format::ARGB1555:
      return SDL_PIXELFORMAT_ARGB1555;
    case neutrino::hal::pixel_format::RGBA5551:
      return SDL_PIXELFORMAT_RGBA5551;
    case neutrino::hal::pixel_format::ABGR1555:
      return SDL_PIXELFORMAT_ABGR1555;
    case neutrino::hal::pixel_format::BGRA5551:
      return SDL_PIXELFORMAT_BGRA5551;
    case neutrino::hal::pixel_format::RGB565:
      return SDL_PIXELFORMAT_RGB565;
    case neutrino::hal::pixel_format::BGR565:
      return SDL_PIXELFORMAT_BGR565;
    case neutrino::hal::pixel_format::RGB24:
      return SDL_PIXELFORMAT_RGB24;
    case neutrino::hal::pixel_format::BGR24:
      return SDL_PIXELFORMAT_BGR24;
    case neutrino::hal::pixel_format::RGB888:
      return SDL_PIXELFORMAT_RGB888;
    case neutrino::hal::pixel_format::RGBX8888:
      return SDL_PIXELFORMAT_RGBX8888;
    case neutrino::hal::pixel_format::BGR888:
      return SDL_PIXELFORMAT_BGR888;
    case neutrino::hal::pixel_format::BGRX8888:
      return SDL_PIXELFORMAT_BGRX8888;
    case neutrino::hal::pixel_format::ARGB8888:
      return SDL_PIXELFORMAT_ARGB8888;
    case neutrino::hal::pixel_format::RGBA8888:
      return SDL_PIXELFORMAT_RGBA8888;
    case neutrino::hal::pixel_format::ABGR8888:
      return SDL_PIXELFORMAT_ABGR8888;
    case neutrino::hal::pixel_format::BGRA8888:
      return SDL_PIXELFORMAT_BGRA8888;
    case neutrino::hal::pixel_format::ARGB2101010:
      return SDL_PIXELFORMAT_ARGB2101010;
    case neutrino::hal::pixel_format::RGBA32:
      return SDL_PIXELFORMAT_RGBA32;
    case neutrino::hal::pixel_format::ARGB32:
      return SDL_PIXELFORMAT_ARGB32;
    case neutrino::hal::pixel_format::BGRA32:
      return SDL_PIXELFORMAT_BGRA32;
    case neutrino::hal::pixel_format::ABGR32:
      return SDL_PIXELFORMAT_ABGR32;
    case neutrino::hal::pixel_format::YV12:
      return SDL_PIXELFORMAT_YV12;
    case neutrino::hal::pixel_format::IYUV:
      return SDL_PIXELFORMAT_IYUV;
    case neutrino::hal::pixel_format::YUY2:
      return SDL_PIXELFORMAT_YUY2;
    case neutrino::hal::pixel_format::UYVY:
      return SDL_PIXELFORMAT_UYVY;
    case neutrino::hal::pixel_format::YVYU:
      return SDL_PIXELFORMAT_YVYU;
    case neutrino::hal::pixel_format::NV12:
      return SDL_PIXELFORMAT_NV12;
    case neutrino::hal::pixel_format::NV21:
      return SDL_PIXELFORMAT_NV21;
    case neutrino::hal::pixel_format::OES:
      return SDL_PIXELFORMAT_EXTERNAL_OES;
    default:
      RAISE_EX("Should not be here");
  }
}

static neutrino::hal::pixel_format::type map_type (unsigned int t) {
  switch (t) {
    case SDL_PIXELTYPE_UNKNOWN :
      return neutrino::hal::pixel_format::type::UNKNOWN;
    case SDL_PIXELTYPE_INDEX1 :
      return neutrino::hal::pixel_format::type::INDEX1;
    case SDL_PIXELTYPE_INDEX4 :
      return neutrino::hal::pixel_format::type::INDEX4;
    case SDL_PIXELTYPE_INDEX8 :
      return neutrino::hal::pixel_format::type::INDEX8;
    case SDL_PIXELTYPE_PACKED8 :
      return neutrino::hal::pixel_format::type::PACKED8;
    case SDL_PIXELTYPE_PACKED16 :
      return neutrino::hal::pixel_format::type::PACKED16;
    case SDL_PIXELTYPE_PACKED32 :
      return neutrino::hal::pixel_format::type::PACKED32;
    case SDL_PIXELTYPE_ARRAYU8 :
      return neutrino::hal::pixel_format::type::ARRAYU8;
    case SDL_PIXELTYPE_ARRAYU16 :
      return neutrino::hal::pixel_format::type::ARRAYU16;
    case SDL_PIXELTYPE_ARRAYU32 :
      return neutrino::hal::pixel_format::type::ARRAYU32;
    case SDL_PIXELTYPE_ARRAYF16 :
      return neutrino::hal::pixel_format::type::ARRAYF16;
    case SDL_PIXELTYPE_ARRAYF32 :
      return neutrino::hal::pixel_format::type::ARRAYF32;
    default:
      RAISE_EX("Should not be here");
  }
}

static const char *to_string (neutrino::hal::pixel_format::type f) {
  switch (f) {
    case neutrino::hal::pixel_format::type::UNKNOWN:
      return "UNKNOWN";
    case neutrino::hal::pixel_format::type::INDEX1:
      return "INDEX1";
    case neutrino::hal::pixel_format::type::INDEX4:
      return "INDEX4";
    case neutrino::hal::pixel_format::type::INDEX8:
      return "INDEX8";
    case neutrino::hal::pixel_format::type::PACKED8:
      return "PACKED8";
    case neutrino::hal::pixel_format::type::PACKED16:
      return "PACKED16";
    case neutrino::hal::pixel_format::type::PACKED32:
      return "PACKED32";
    case neutrino::hal::pixel_format::type::ARRAYU8:
      return "ARRAYU8";
    case neutrino::hal::pixel_format::type::ARRAYU16:
      return "ARRAYU16";
    case neutrino::hal::pixel_format::type::ARRAYU32:
      return "ARRAYU32";
    case neutrino::hal::pixel_format::type::ARRAYF16:
      return "ARRAYF16";
    case neutrino::hal::pixel_format::type::ARRAYF32:
      return "ARRAYF32";
    default:
      RAISE_EX("Should not be here");
  }
}

static neutrino::hal::pixel_format::order map_order (unsigned int t) {
  switch (t) {
    case SDL_BITMAPORDER_NONE:
      return neutrino::hal::pixel_format::order::NONE;
    case SDL_BITMAPORDER_1234:
      return neutrino::hal::pixel_format::order::ORDER_1234;
    case SDL_BITMAPORDER_4321:
      return neutrino::hal::pixel_format::order::ORDER_4321;
    default:
      RAISE_EX("Should not be here");
  }
}

static const char *to_string (neutrino::hal::pixel_format::order o) {
  switch (o) {
    case neutrino::hal::pixel_format::order::NONE:
      return "NONE";
    case neutrino::hal::pixel_format::order::ORDER_4321:
      return "ORDER_4321";
    case neutrino::hal::pixel_format::order::ORDER_1234:
      return "ORDER_1234";
    default:
      RAISE_EX("Should not be here");
  }
}

static const char *to_string (neutrino::hal::pixel_format::component_order o) {
  switch (o) {
    case neutrino::hal::pixel_format::component_order::NONE:
      return "NONE";
    case neutrino::hal::pixel_format::component_order::XRGB:
      return "XRGB";
    case neutrino::hal::pixel_format::component_order::RGBX:
      return "RGBX";
    case neutrino::hal::pixel_format::component_order::ARGB:
      return "ARGB";
    case neutrino::hal::pixel_format::component_order::RGBA:
      return "RGBA";
    case neutrino::hal::pixel_format::component_order::XBGR:
      return "XBGR";
    case neutrino::hal::pixel_format::component_order::BGRX:
      return "BGRX";
    case neutrino::hal::pixel_format::component_order::ABGR:
      return "ABGR";
    case neutrino::hal::pixel_format::component_order::BGRA:
      return "BGRA";
    default:
      RAISE_EX("Should not be here");
  }
}

static neutrino::hal::pixel_format::component_order map_component_order (unsigned int o) {
  switch (o) {
    case SDL_PACKEDORDER_NONE:
      return neutrino::hal::pixel_format::component_order::NONE;
    case SDL_PACKEDORDER_XRGB:
      return neutrino::hal::pixel_format::component_order::XRGB;
    case SDL_PACKEDORDER_RGBX:
      return neutrino::hal::pixel_format::component_order::RGBX;
    case SDL_PACKEDORDER_ARGB:
      return neutrino::hal::pixel_format::component_order::ARGB;
    case SDL_PACKEDORDER_RGBA:
      return neutrino::hal::pixel_format::component_order::RGBA;
    case SDL_PACKEDORDER_XBGR:
      return neutrino::hal::pixel_format::component_order::XBGR;
    case SDL_PACKEDORDER_BGRX:
      return neutrino::hal::pixel_format::component_order::BGRX;
    case SDL_PACKEDORDER_ABGR:
      return neutrino::hal::pixel_format::component_order::ABGR;
    case SDL_PACKEDORDER_BGRA:
      return neutrino::hal::pixel_format::component_order::BGRA;
    default:
      RAISE_EX("Should not be here");
  }
}

static neutrino::hal::pixel_format::array_order map_array_order (unsigned int o) {
  switch (o) {
    case SDL_ARRAYORDER_NONE:
      return neutrino::hal::pixel_format::array_order::NONE;
    case SDL_ARRAYORDER_RGB:
      return neutrino::hal::pixel_format::array_order::RGB;
    case SDL_ARRAYORDER_RGBA:
      return neutrino::hal::pixel_format::array_order::RGBA;
    case SDL_ARRAYORDER_ARGB:
      return neutrino::hal::pixel_format::array_order::ARGB;
    case SDL_ARRAYORDER_BGR:
      return neutrino::hal::pixel_format::array_order::BGR;
    case SDL_ARRAYORDER_BGRA:
      return neutrino::hal::pixel_format::array_order::BGRA;
    case SDL_ARRAYORDER_ABGR:
      return neutrino::hal::pixel_format::array_order::ABGR;
    default:
      RAISE_EX("Should not be here");
  }
}

static const char *to_string (neutrino::hal::pixel_format::array_order o) {
  switch (o) {
    case neutrino::hal::pixel_format::array_order::NONE:
      return "NONE";
    case neutrino::hal::pixel_format::array_order::RGB:
      return "RGB";
    case neutrino::hal::pixel_format::array_order::RGBA:
      return "RGBA";
    case neutrino::hal::pixel_format::array_order::ARGB:
      return "ARGB";
    case neutrino::hal::pixel_format::array_order::BGR:
      return "BGR";
    case neutrino::hal::pixel_format::array_order::BGRA:
      return "BGRA";
    case neutrino::hal::pixel_format::array_order::ABGR:
      return "ABGR";
    default:
      RAISE_EX("Should not be here");
  }
}

static const char *to_string (neutrino::hal::pixel_format::layout o) {
  switch (o) {
    case neutrino::hal::pixel_format::layout::NONE:
      return "NONE";
    case neutrino::hal::pixel_format::layout::LAYOUT_332:
      return "LAYOUT_332";
    case neutrino::hal::pixel_format::layout::LAYOUT_4444:
      return "LAYOUT_4444";
    case neutrino::hal::pixel_format::layout::LAYOUT_1555:
      return "LAYOUT_1555";
    case neutrino::hal::pixel_format::layout::LAYOUT_5551:
      return "LAYOUT_5551";
    case neutrino::hal::pixel_format::layout::LAYOUT_565:
      return "LAYOUT_565";
    case neutrino::hal::pixel_format::layout::LAYOUT_8888:
      return "LAYOUT_8888";
    case neutrino::hal::pixel_format::layout::LAYOUT_2101010:
      return "LAYOUT_2101010";
    case neutrino::hal::pixel_format::layout::LAYOUT_1010102:
      return "LAYOUT_1010102";
    default:
      RAISE_EX("Should not be here");
  }
}

static neutrino::hal::pixel_format::layout map_layout (unsigned int o) {
  switch (o) {
    case SDL_PACKEDLAYOUT_NONE:
      return neutrino::hal::pixel_format::layout::NONE;
    case SDL_PACKEDLAYOUT_332:
      return neutrino::hal::pixel_format::layout::LAYOUT_332;
    case SDL_PACKEDLAYOUT_4444:
      return neutrino::hal::pixel_format::layout::LAYOUT_4444;
    case SDL_PACKEDLAYOUT_1555:
      return neutrino::hal::pixel_format::layout::LAYOUT_1555;
    case SDL_PACKEDLAYOUT_5551:
      return neutrino::hal::pixel_format::layout::LAYOUT_5551;
    case SDL_PACKEDLAYOUT_565:
      return neutrino::hal::pixel_format::layout::LAYOUT_565;
    case SDL_PACKEDLAYOUT_8888:
      return neutrino::hal::pixel_format::layout::LAYOUT_8888;
    case SDL_PACKEDLAYOUT_2101010:
      return neutrino::hal::pixel_format::layout::LAYOUT_2101010;
    case SDL_PACKEDLAYOUT_1010102:
      return neutrino::hal::pixel_format::layout::LAYOUT_1010102;
    default:
      RAISE_EX("Should not be here");
  }
}

namespace neutrino::hal {
  // ----------------------------------------------------------------------------------------------
  pixel_format pixel_format::make_8bit () {
    return pixel_format (pixel_format::INDEX8);
  }
  // ----------------------------------------------------------------------------------------------

  pixel_format pixel_format::make_rgba_32bit () {
    return pixel_format (pixel_format::RGBA32);
  }
  // ----------------------------------------------------------------------------------------------
  pixel_format::pixel_format (const description &d)
      : m_value (SDL_MasksToPixelFormatEnum (static_cast<int>(d.bpp), d.rmask, d.gmask, d.bmask, d.amask)) {
    if (m_value == SDL_PIXELFORMAT_UNKNOWN) {
      RAISE_EX("Can not create pixel format from the provided parameters");
    }
  }

  pixel_format::pixel_format (uint8_t bpp, uint32_t rmask, uint32_t gmask, uint32_t bmask, uint32_t amask)
      : m_value (SDL_MasksToPixelFormatEnum (static_cast<int>(bpp), rmask, gmask, bmask, amask)) {
    if (m_value == SDL_PIXELFORMAT_UNKNOWN) {
      RAISE_EX("Can not create pixel format from the provided parameters");
    }
  }

  // --------------------------------------------------------------------------------
  pixel_format::pixel_format (std::uint32_t f)
      : m_value (f) {

  }
  // --------------------------------------------------------------------------------

  pixel_format::pixel_format (format f)
      : m_value (map_format (f)) {

  }
  // --------------------------------------------------------------------------------

  pixel_format::operator std::uint32_t () const noexcept {
    return value ();
  }
  // --------------------------------------------------------------------------------
  uint32_t pixel_format::value () const noexcept {
    return m_value;
  }
  // --------------------------------------------------------------------------------

  pixel_format::format pixel_format::get_format () const noexcept {
    return from_format (m_value);
  }
  // --------------------------------------------------------------------------------

  pixel_format::type pixel_format::get_type () const noexcept {
    return map_type (SDL_PIXELFLAG(m_value));
  }
  // --------------------------------------------------------------------------------

  pixel_format::order pixel_format::get_order () const noexcept {
    return map_order (SDL_PIXELORDER(m_value));
  }
  // --------------------------------------------------------------------------------

  pixel_format::component_order pixel_format::get_component_order () const noexcept {
    return map_component_order (SDL_PIXELORDER(m_value));
  }
  // --------------------------------------------------------------------------------

  pixel_format::array_order pixel_format::get_array_order () const noexcept {
    return map_array_order (SDL_PIXELORDER(m_value));
  }
  // --------------------------------------------------------------------------------

  pixel_format::layout pixel_format::get_layout_order () const noexcept {
    return map_layout (SDL_PIXELLAYOUT(m_value));
  }
  // --------------------------------------------------------------------------------

  std::uint8_t pixel_format::get_bits_per_pixels () const noexcept {
    return SDL_BITSPERPIXEL(m_value);
  }
  // --------------------------------------------------------------------------------

  std::uint8_t pixel_format::get_bytes_per_pixels () const noexcept {
    return SDL_BYTESPERPIXEL(m_value);
  }
  // --------------------------------------------------------------------------------

  bool pixel_format::is_indexed () const noexcept {
    return SDL_ISPIXELFORMAT_INDEXED(m_value) != 0;
  }
  // --------------------------------------------------------------------------------

  bool pixel_format::is_array () const noexcept {
    return SDL_ISPIXELFORMAT_ARRAY(m_value) != 0;
  }
  // --------------------------------------------------------------------------------

  bool pixel_format::is_alpha () const noexcept {
    return SDL_ISPIXELFORMAT_ALPHA(m_value) != 0;
  }
  // --------------------------------------------------------------------------------

  bool pixel_format::is_fourcc () const noexcept {
    return SDL_ISPIXELFORMAT_FOURCC(m_value) != 0;
  }
  // --------------------------------------------------------------------------------

  bool pixel_format::is_packed () const noexcept {
    return SDL_ISPIXELFORMAT_PACKED(m_value);
  }
  // --------------------------------------------------------------------------------

  pixel_format::description pixel_format::get_mask () const {
    int bpp;
    uint32_t r, g, b, a;
    if (SDL_TRUE == SDL_PixelFormatEnumToMasks (m_value, &bpp, &r, &g, &b, &a)) {
      return {static_cast<uint8_t>(bpp), r, g, b, a};
    }
    RAISE_EX("No pixel format conversion is possible");
  }

  std::ostream &operator<< (std::ostream &os, pixel_format::format f) {
    os << to_string (f);
    return os;
  }

  std::ostream &operator<< (std::ostream &os, pixel_format::layout f) {
    os << to_string (f);
    return os;
  }
  std::ostream &operator<< (std::ostream &os, pixel_format::array_order f) {
    os << to_string (f);
    return os;
  }
  std::ostream &operator<< (std::ostream &os, pixel_format::component_order f) {
    os << to_string (f);
    return os;
  }
  std::ostream &operator<< (std::ostream &os, pixel_format::order f) {
    os << to_string (f);
    return os;
  }
  std::ostream &operator<< (std::ostream &os, pixel_format::type f) {
    os << to_string (f);
    return os;
  }
}