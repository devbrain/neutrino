set(BUILD_SHARED_LIBS TRUE CACHE INTERNAL "")
set(sdlpp_ENABLE_TESTING FALSE CACHE INTERNAL "")
fetchcontent_declare(
        SDLPP_EXT
        GIT_REPOSITORY https://github.com/devbrain/lib_sdlpp.git
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
        GIT_TAG master
        TIMEOUT 10
        UPDATE_COMMAND ""
)

fetchcontent_makeavailable(SDLPP_EXT)

