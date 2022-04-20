my_add_library_static(kernel
        SOURCES

        ${PROJECT_INCLUDE}/neutrino/kernel/application.hh
        application.cc

        ${PROJECT_INCLUDE}/neutrino/kernel/application_description.hh
        application_description.cc

        main_window.hh
        main_window.cc

        ${PROJECT_INCLUDE}/neutrino/kernel/hdi/events.hh
        hdi/events.cc
        hdi/kbd_actions.cc
        hdi/kbd_actions.hh
        hdi/kbd_mapper.cc
        hdi/kbd_mapper.hh
        ${PROJECT_INCLUDE}/neutrino/kernel/hdi/input_config.hh
        hdi/input_config.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/hdi/pointer_config.hh
        hdi/pointer_config.cc
        hdi/pointer_mapper.cc
        hdi/pointer_mapper.hh

        ${PROJECT_INCLUDE}/neutrino/kernel/gfx/tilesheet_info.hh
        gfx/tilesheet_info.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/gfx/tilesheet.hh
        gfx/tilesheet.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/gfx/grid_image.hh
        gfx/grid_image.cc

        DEPENDS_PUBLIC
            neutrino::hal
            neutrino::utils
        )
