set(BOX2D_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE)

fetchcontent_declare(
        BOX2d_Ext
        GIT_REPOSITORY "https://github.com/erincatto/Box2D.git"
        GIT_TAG v2.4.1     # or a particular branch, tag or commit
        GIT_SHALLOW FALSE  # ensure submodules are checked out
)
fetchcontent_makeavailable(BOX2d_Ext)


