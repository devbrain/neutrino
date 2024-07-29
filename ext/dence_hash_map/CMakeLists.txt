set(DENSE_HASH_MAP_BUILD_TEST FALSE CACHE INTERNAL "")
fetchcontent_declare(
        DHM_EXT
        GIT_REPOSITORY https://github.com/devbrain/dense_hash_map.git
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
        GIT_TAG better_cmake
        TIMEOUT 10
        UPDATE_COMMAND ""
)

fetchcontent_makeavailable(DHM_EXT)

