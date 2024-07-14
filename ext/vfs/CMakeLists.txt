set(BUILD_SHARED_LIBS TRUE CACHE INTERNAL "")
set(VFS_ENABLE_TESTING FALSE CACHE INTERNAL "")
fetchcontent_declare(
        VFS_EXT
        GIT_REPOSITORY https://github.com/devbrain/vfs.git
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
        GIT_TAG master
        TIMEOUT 10
        UPDATE_COMMAND ""
)

fetchcontent_makeavailable(VFS_EXT)

