

if (VULKAN_FOUND)
    message(STATUS "VULKAN found")
    set(VULKAN_TARGET Vulkan::Vulkan)
endif ()
if (OPENGL_FOUND)
    message(STATUS "OPENGL found")
endif ()

set(FONT_DATA
        IBM_PC_V1_8x8.bin
        IBM_VGA_8x14.bin
        IBM_VGA_8x16.bin
)

include(bin2c)
set(GENERATED_FONT_HEADERS)
foreach (FILE ${FONT_DATA})
    set(PATH "video/fonts/${FILE}")
    bin2c(${PATH} generated)
    list(APPEND GENERATED_FONT_HEADERS ${generated})
endforeach ()

my_add_library_static(hal
        SOURCES

        ${PROJECT_INCLUDE}/neutrino/hal/video/window.hh
        video/windows/window.cc
        video/windows/window_impl.hh
        video/windows/windows_manager.hh
        video/windows/windows_manager.cc

        message_broker.hh
        message_broker.cc

        events/events_s11n.cc
        ${PROJECT_INCLUDE}/neutrino/hal/events/events_s11n.hh
        ${PROJECT_INCLUDE}/neutrino/hal/events/events.hh
        ${PROJECT_INCLUDE}/neutrino/hal/events/scancodes.hh

        events/events_mapper.hh
        events/events_mapper.cc
        events/scancode_mapper.cc
        events/scancode_mapper.hh
        events/events_broker.cc
        ${PROJECT_INCLUDE}/neutrino/hal/events/events_broker.hh

        application.cc
        ${PROJECT_INCLUDE}/neutrino/hal/application.hh

        ${PROJECT_INCLUDE}/neutrino/hal/sdl_event_hook.hh
        sdl_event_hook.cc

        system.cc
        ${PROJECT_INCLUDE}/neutrino/hal/system.hh

        video/surface.cc
        ${PROJECT_INCLUDE}/neutrino/hal/video/surface.hh
        video/surface_impl.hh

        video/palette.cc
        ${PROJECT_INCLUDE}/neutrino/hal/video/palette.hh
        video/palette_impl.hh

        video/pixel_format.cc
        ${PROJECT_INCLUDE}/neutrino/hal/video/pixel_format.hh

        cast.hh
        ${PROJECT_INCLUDE}/neutrino/hal/video/color.hh
        ${PROJECT_INCLUDE}/neutrino/hal/video/blend_mode.hh

        ${PROJECT_INCLUDE}/neutrino/hal/video/image_loader.hh
        video/image_loader.cc
        video/thirdparty/SDL_stbimage.h
        video/thirdparty/stb_image.h
        video/thirdparty/stb_image.c
        video/thirdparty/stb_image_write.h
        video/thirdparty/stb_image_write.c
        video/thirdparty/SDL_lbm.h
        video/thirdparty/SDL_pcx.h
        video/thirdparty/SDL_ttf.h
        video/thirdparty/SDL_ttf.c
        video/stb_image_loader.cc
        video/stb_image_loader.hh
        video/image_writer.cc
        video/image_writer.hh

        video/thirdparty/pngpp/color.hpp
        video/thirdparty/pngpp/config.hpp
        video/thirdparty/pngpp/consumer.hpp
        video/thirdparty/pngpp/convert_color_space.hpp
        video/thirdparty/pngpp/end_info.hpp
        video/thirdparty/pngpp/error.hpp
        video/thirdparty/pngpp/ga_pixel.hpp
        video/thirdparty/pngpp/generator.hpp
        video/thirdparty/pngpp/gray_pixel.hpp
        video/thirdparty/pngpp/image.hpp
        video/thirdparty/pngpp/image_info.hpp
        video/thirdparty/pngpp/index_pixel.hpp
        video/thirdparty/pngpp/info_base.hpp
        video/thirdparty/pngpp/info.hpp
        video/thirdparty/pngpp/io_base.hpp
        video/thirdparty/pngpp/packed_pixel.hpp
        video/thirdparty/pngpp/palette.hpp
        video/thirdparty/pngpp/pixel_buffer.hpp
        video/thirdparty/pngpp/pixel_traits.hpp
        video/thirdparty/pngpp/png.hpp
        video/thirdparty/pngpp/reader.hpp
        video/thirdparty/pngpp/require_color_space.hpp
        video/thirdparty/pngpp/rgba_pixel.hpp
        video/thirdparty/pngpp/rgb_pixel.hpp
        video/thirdparty/pngpp/solid_pixel_buffer.hpp
        video/thirdparty/pngpp/streaming_base.hpp
        video/thirdparty/pngpp/tRNS.hpp
        video/thirdparty/pngpp/types.hpp
        video/thirdparty/pngpp/writer.hpp

        video/thirdparty/libamivideo/amivideotypes.h
        video/thirdparty/libamivideo/palette.c
        video/thirdparty/libamivideo/palette.h
        video/thirdparty/libamivideo/screen.c
        video/thirdparty/libamivideo/screen.h
        video/thirdparty/libamivideo/viewportmode.c
        video/thirdparty/libamivideo/viewportmode.h
        video/amiga/iff/fourcc.cc
        video/amiga/iff/fourcc.hh
        video/amiga/iff/iff_events.cc
        video/amiga/iff/iff_events.hh
        video/amiga/iff/iff_parser.hh
        video/amiga/iff/ea/ea.hh
        video/amiga/iff/ea/ea_events.cc
        video/amiga/iff/ea/ea_events.hh
        video/amiga/amiga_chunks.cc
        video/amiga/amiga_chunks.hh
        video/amiga/amiga_image.cc
        video/amiga/amiga_image.hh
        video/amiga/lbm.cc
        video/amiga/lbm.hh

        ios_rwops.hh

        video/thirdparty/gfx/SDL2_gfxPrimitives.c
        video/thirdparty/gfx/SDL2_gfxPrimitives.h
        video/thirdparty/gfx/SDL2_gfxPrimitives_font.h
        video/thirdparty/gfx/SDL2_rotozoom.c
        video/thirdparty/gfx/SDL2_rotozoom.h


        video/texture.cc
        video/texture_impl.hh
        ${PROJECT_INCLUDE}/neutrino/hal/video/texture.hh

        ${PROJECT_INCLUDE}/neutrino/hal/video/renderer_utils.hh
        video/renderer_utils.cc

        video/renderer.cc
        video/renderer_impl.hh
        ${PROJECT_INCLUDE}/neutrino/hal/video/renderer.hh

        sdl/io.hh
        sdl/call.hh
        sdl/timers.hh
        sdl/render.hh
        sdl/geometry.hh
        sdl/iterator.hh
        sdl/object.hh
        sdl/color.hh
        sdl/texture.hh
        sdl/surface.hh
        sdl/rtc.hh
        sdl/pixel_format.hh
        sdl/system.hh
        sdl/sdl2.hh
        sdl/events/system_events.hh
        sdl/events/event_types.hh
        sdl/events/events_dispatcher.hh
        sdl/palette.hh
        sdl/window.hh

        ${GENERATED_FONT_HEADERS}
        ${PROJECT_INCLUDE}/neutrino/hal/video/fonts/rom_fonts.hh
        video/fonts/rom_fonts.cc

        DEPENDS_PUBLIC
        neutrino::utils
        neutrino::math
        thirdparty::glm
        thirdparty::bitflags
        thirdparty::freetype
        thirdparty::png
        thirdparty::sdl2
        ${VULKAN_LIBS}
        )

