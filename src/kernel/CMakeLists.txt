my_add_library_static(kernel
        SOURCES

        ${PROJECT_INCLUDE}/neutrino/kernel/application.hh
        application.cc

        ${PROJECT_INCLUDE}/neutrino/kernel/events.hh

        ${PROJECT_INCLUDE}/neutrino/kernel/system.hh
        system.cc
        systems_manager.cc
        systems_manager.hh
        main_window.cc
        main_window.hh

        ${PROJECT_INCLUDE}/neutrino/kernel/systems/base_input_system.hh
        ${PROJECT_INCLUDE}/neutrino/kernel/systems/input_system.hh
        systems/base_input_system.cc

        ${PROJECT_INCLUDE}/neutrino/kernel/systems/video/video_system.hh
        systems/video/video_system.cc

        ${PROJECT_INCLUDE}/neutrino/kernel/systems/video/accel_renderer_video_system.hh
        systems/video/accel_renderer_video_system.cc

        ${PROJECT_INCLUDE}/neutrino/kernel/systems/video/vga256/vga256_system.hh
        systems/video/vga256/vga256_system.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/systems/video/vga256/vga256.hh
        systems/video/vga256/vga256.cc

        ${PROJECT_INCLUDE}/neutrino/kernel/systems/video/opengl_video_system.hh



        DEPENDS_PUBLIC
            neutrino::hal
            neutrino::utils
        )
