set(PNG_TESTS OFF CACHE INTERNAL "")
set(PNG_BUILD_ZLIB OFF CACHE INTERNAL "")
set(PNG_SHARED OFF CACHE INTERNAL "")
set(PNG_STATIC  ON CACHE INTERNAL "")
set(PNG_EXECUTABLES OFF CACHE INTERNAL "")


FetchContent_Declare(png
        GIT_REPOSITORY https://github.com/glennrp/libpng.git
        GIT_TAG master
        UPDATE_COMMAND ""
        PATCH_COMMAND  git apply ${CMAKE_CURRENT_SOURCE_DIR}/png.patch || true
)

FetchContent_MakeAvailable(png)
add_library(PNG::png ALIAS png_static)




