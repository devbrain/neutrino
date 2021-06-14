my_add_library_static(engine
        SOURCES

            observer.cc

            message_broker.cc

            input_publisher.hh

            windows_manager.cc
            windows_manager.hh

            windows/basic_window.cc
            windows/engine_window.cc
            windows/window_2d.cc
            windows/window_opengl.cc
            windows/window_vulkan.cc

            events/events_mapper.cc
            events/events_mapper.hh
            events/events_s11n.cc

            application/basic_application.cc

            ${PROJECT_INCLUDE}/neutrino/engine/application.hh

            ${PROJECT_INCLUDE}/neutrino/engine/observer.hh
            ${PROJECT_INCLUDE}/neutrino/engine/message_broker.hh
            ${PROJECT_INCLUDE}/neutrino/engine/window.hh

            ${PROJECT_INCLUDE}/neutrino/engine/windows/basic_window.hh
            ${PROJECT_INCLUDE}/neutrino/engine/windows/engine_window.hh
            ${PROJECT_INCLUDE}/neutrino/engine/windows/window_opengl.hh
            ${PROJECT_INCLUDE}/neutrino/engine/windows/window_vulkan.hh
            ${PROJECT_INCLUDE}/neutrino/engine/windows/window_2d.hh

            ${PROJECT_INCLUDE}/neutrino/engine/events/events.hh
            ${PROJECT_INCLUDE}/neutrino/engine/events/events_s11n.hh

            ${PROJECT_INCLUDE}/neutrino/engine/application/basic_application.hh
            ${PROJECT_INCLUDE}/neutrino/engine/application/single_window_application.hh
        DEPENDS_PUBLIC
            neutrino::sdl
            neutrino::utils
            thirdparty::sdl2
            thirdparty::bitflags
            thirdparty::glm
        )