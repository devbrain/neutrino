my_add_library_static(engine
        SOURCES
            ${PROJECT_INCLUDE}/neutrino/engine/application.hh
            application.cc

        DEPENDS_PUBLIC
            neutrino::hal
            neutrino::utils
        )