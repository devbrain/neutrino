set(DOCTEST_WITH_TESTS OFF CACHE INTERNAL "")
set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE INTERNAL "")
FetchContent_Declare(
        DOCTEST
        GIT_REPOSITORY "https://github.com/onqtam/doctest.git"
)
FetchContent_MakeAvailable(DOCTEST)
add_library(thirdparty::doctest ALIAS doctest)
