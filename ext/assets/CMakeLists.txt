set(BUILD_SHARED_LIBS TRUE CACHE INTERNAL "")
set(ASSETS_ENABLE_TESTING FALSE CACHE INTERNAL "")
fetchcontent_declare(
        ASSETS_EXT
        GIT_REPOSITORY https://github.com/devbrain/lib_assets.git
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
        GIT_TAG master
        TIMEOUT 10
        UPDATE_COMMAND ""
)

fetchcontent_makeavailable(ASSETS_EXT)

