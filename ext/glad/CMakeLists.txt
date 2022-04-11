fetchcontent_declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG master
)

fetchcontent_makeavailable(glad)


add_library(thirdparty::glad ALIAS glad)
