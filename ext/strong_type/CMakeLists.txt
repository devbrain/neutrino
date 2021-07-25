set(BITFLAGS_BUILD_TESTS OFF CACHE INTERNAL "")
set(BITFLAGS_BUILD_SAMPLES OFF CACHE INTERNAL "")
set(BITFLAGS_BUILD_BENCHMARK OFF CACHE INTERNAL "")

FetchContent_Declare (
        STRONG_TYPE
        GIT_TAG main
        GIT_REPOSITORY https://github.com/rollbear/strong_type.git
)

FetchContent_MakeAvailable(STRONG_TYPE)
add_library(thirdparty::strong_type ALIAS strong_type)
set_target_properties(strong_type PROPERTIES FOLDER "ThirdParty")
