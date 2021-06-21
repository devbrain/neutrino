my_add_library_static(engine
        SOURCES

            observer.cc

            message_broker.cc

            input_publisher.hh

            windows_manager.cc
            windows_manager.hh

            windows/basic_window.cc
            windows/engine_window.cc

            events/events_mapper.cc
            events/events_mapper.hh
            events/events_s11n.cc

            application/basic_application.cc

            renderer/vga_renderer.cc
            renderer/basic_renderer.cc
            renderer/renderer_2d.cc
            renderer/opengl_renderer.cc

            ${PROJECT_INCLUDE}/neutrino/engine/application.hh

            ${PROJECT_INCLUDE}/neutrino/engine/observer.hh
            ${PROJECT_INCLUDE}/neutrino/engine/message_broker.hh

            ${PROJECT_INCLUDE}/neutrino/engine/windows/basic_window.hh
            ${PROJECT_INCLUDE}/neutrino/engine/windows/engine_window.hh

            ${PROJECT_INCLUDE}/neutrino/engine/events/events.hh
            ${PROJECT_INCLUDE}/neutrino/engine/events/events_s11n.hh

            ${PROJECT_INCLUDE}/neutrino/engine/application/basic_application.hh
            ${PROJECT_INCLUDE}/neutrino/engine/application/generic_application.hh
            ${PROJECT_INCLUDE}/neutrino/engine/application/neutrino_application.hh
            ${PROJECT_INCLUDE}/neutrino/engine/application/oldschool_demo.hh
            application/oldschool_demo.cc

            ${PROJECT_INCLUDE}/neutrino/engine/renderer/basic_renderer.hh
            ${PROJECT_INCLUDE}/neutrino/engine/renderer/vga_renderer.hh
            ${PROJECT_INCLUDE}/neutrino/engine/renderer/renderer_2d.hh
            ${PROJECT_INCLUDE}/neutrino/engine/renderer/opengl_renderer.hh

            thirdparty/stb_image.c
            thirdparty/stb_image.h
            thirdparty/SDL_stbimage.h

            ${PROJECT_INCLUDE}/neutrino/engine/systems/video/video_system.hh
            ${PROJECT_INCLUDE}/neutrino/engine/systems/video/oldschool.hh

            ${PROJECT_INCLUDE}/neutrino/engine/systems/video/retro/retro.hh
            systems/video/retro/retro.cc
            systems/video/oldschool.cc

        DEPENDS_PUBLIC
            neutrino::sdl
            neutrino::utils
            thirdparty::sdl2
            thirdparty::bitflags
            thirdparty::glm
        )