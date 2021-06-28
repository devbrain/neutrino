set(ENABLE_LIB_ONLY ON CACHE INTERNAL "")
set(ENABLE_STATIC_LIB ON CACHE INTERNAL "")
set(ENABLE_SHARED_LIB OFF CACHE INTERNAL "")
set(ENABLE_WERROR OFF CACHE INTERNAL "")

FetchContent_Declare(bz2
        GIT_REPOSITORY  https://gitlab.com/federicomenaquintero/bzip2
        GIT_TAG
)

FetchContent_MakeAvailable(bz2)
add_library(BZIP2::bz2 ALIAS bz2_static)


