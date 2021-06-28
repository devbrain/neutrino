my_add_library_static(hal
        SOURCES

        ${PROJECT_INCLUDE}/neutrino/hal/window.hh
        windows/window.cc
        windows/window_impl.hh
        windows/windows_manager.hh
        windows/windows_manager.cc

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

        surface.cc
        ${PROJECT_INCLUDE}/neutrino/hal/surface.hh
        surface_impl.hh

        pixel_format.cc
        ${PROJECT_INCLUDE}/neutrino/hal/pixel_format.hh

        cast.hh
        ${PROJECT_INCLUDE}/neutrino/hal/color.hh

        DEPENDS_PUBLIC
            neutrino::math
            neutrino::sdl
            thirdparty::glm
            thirdparty::bitflags
        )