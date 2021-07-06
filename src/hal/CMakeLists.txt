

if (VULKAN_FOUND)
    message(STATUS "VULKAN found")
    set(VULKAN_TARGET Vulkan::Vulkan)
endif()
if (OPENGL_FOUND)
    message(STATUS "OPENGL found")
endif()

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

        system.cc
        ${PROJECT_INCLUDE}/neutrino/hal/system.hh

        video/surface.cc
        ${PROJECT_INCLUDE}/neutrino/hal/video/surface.hh
        video/surface_impl.hh

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
        video/thirdparty/SDL_lbm.h
        video/thirdparty/SDL_pcx.h
        video/stb_image_loader.cc
        ios_rwops.hh

        video/thirdparty/gfx/SDL2_gfxPrimitives.c
        video/thirdparty/gfx/SDL2_gfxPrimitives.h
        video/thirdparty/gfx/SDL2_gfxPrimitives_font.h
        video/thirdparty/gfx/SDL2_rotozoom.c
        video/thirdparty/gfx/SDL2_rotozoom.h

        video/texture.cc
        video/texture_impl.hh
        ${PROJECT_INCLUDE}/neutrino/hal/video/texture.hh

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

        DEPENDS_PUBLIC
            neutrino::math
            thirdparty::glm
            thirdparty::bitflags
            thirdparty::sdl2
            ${VULKAN_LIBS}
        )

