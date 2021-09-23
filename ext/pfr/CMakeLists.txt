set(BUILD_TESTING OFF CACHE INTERNAL "")
fetchcontent_declare(
        PFR
        GIT_REPOSITORY "https://github.com/boostorg/pfr.git"
)
fetchcontent_makeavailable(PFR)
add_library(thirdparty::pfr ALIAS boost_pfr)
