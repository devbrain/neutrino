my_add_library_headers(math
        SOURCES
        ${PROJECT_INCLUDE}/neutrino/math/point.hh
        ${PROJECT_INCLUDE}/neutrino/math/rect.hh
        DEPENDS
        thirdparty::glm
        )