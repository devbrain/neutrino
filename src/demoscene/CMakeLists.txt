my_add_library_static(demoscene
        SOURCES
            demoscene.cc
            ${PROJECT_INCLUDE}/neutrino/demoscene/demoscene.hh

            video.cc
            video.hh
            main_scene.cc
            main_scene.hh
        DEPENDS_PUBLIC
            neutrino::engine
)