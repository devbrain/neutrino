set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
FetchContent_Declare(zlib
        GIT_REPOSITORY  https://github.com/madler/zlib.git
        GIT_TAG         cacf7f1d4e3d44d871b605da3b647f07d718623f
        UPDATE_COMMAND ""
        PATCH_COMMAND   git apply ${CMAKE_CURRENT_SOURCE_DIR}/zlib.patch || true
)

FetchContent_MakeAvailable(zlib)
add_library(thirdparty::zlib ALIAS zlibstatic)


