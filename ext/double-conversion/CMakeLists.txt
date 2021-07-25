set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")

FetchContent_Declare (
        DBC
        GIT_REPOSITORY https://github.com/google/double-conversion.git
)

FetchContent_MakeAvailable(DBC)
add_library(thirdparty::double-conversion ALIAS double-conversion)


SET_TARGET_PROPERTIES(double-conversion PROPERTIES FOLDER "ThirdParty")
