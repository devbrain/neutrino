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

        ${PROJECT_INCLUDE}/neutrino/kernel/ecs/sparse_set.hh
        ${PROJECT_INCLUDE}/neutrino/kernel/ecs/sparse_map.hh
        ${PROJECT_INCLUDE}/neutrino/kernel/ecs/registry.hh
        ${PROJECT_INCLUDE}/neutrino/kernel/ecs/id_factory.hh
        ecs/registry.cc
        ecs/id_factory.cc

        gfx/tiled_image.cc
        gfx/tiled_image.hh
        gfx/texture_atlas.hh
        gfx/texture_atlas.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/gfx/world_renderer.hh
        gfx/world_renderer.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/gfx/world_window.hh
        gfx/world_window.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/gfx/grid.hh
        gfx/grid.cc

        gfx/animation_state.hh
        gfx/animation_state.cc

        DEPENDS_PUBLIC
            neutrino::assets
            neutrino::hal
            neutrino::math
            neutrino::utils

        )
