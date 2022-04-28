fetchcontent_declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG master
        UPDATE_COMMAND ""
)

fetchcontent_makeavailable(glad)


add_library(thirdparty::glad ALIAS glad)
set_thirdparty(glad)
set_thirdparty(glad-generate-files)