my_add_library_static(engine
        SOURCES
            ${PROJECT_INCLUDE}/neutrino/engine/application_monitor.hh
            application_monitor.cc

            ${PROJECT_INCLUDE}/neutrino/engine/application.hh
            application.cc

            ${PROJECT_INCLUDE}/neutrino/engine/graphics/abstract_context.hh
            graphics/abstract_context.cc

            ${PROJECT_INCLUDE}/neutrino/engine/graphics/accelerated_2d_context.hh
            graphics/accelerated_2d_context.cc

            window.hh
            window.cc

            ${PROJECT_INCLUDE}/neutrino/engine/scene_manager.hh
            scene_manager.cc

            ${PROJECT_INCLUDE}/neutrino/engine/scene.hh
            scene.cc

            ${PROJECT_INCLUDE}/neutrino/engine/events.hh
        DEPENDS_PUBLIC
            neutrino::hal
            neutrino::utils
        )
