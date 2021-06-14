set(BITFLAGS_BUILD_TESTS OFF CACHE INTERNAL "")
set(BITFLAGS_BUILD_SAMPLES OFF CACHE INTERNAL "")
set(BITFLAGS_BUILD_BENCHMARK OFF CACHE INTERNAL "")

FetchContent_Declare (
        BITFLAGS
        GIT_REPOSITORY https://github.com/m-peko/bitflags.git
)

FetchContent_MakeAvailable(BITFLAGS)
add_library(thirdparty::bitflags ALIAS bitflags)
set_target_properties(bitflags PROPERTIES FOLDER "ThirdParty")
