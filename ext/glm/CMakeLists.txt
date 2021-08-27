fetchcontent_declare(
        GLM
        GIT_REPOSITORY "https://github.com/g-truc/glm.git"
)
fetchcontent_makeavailable(GLM)
add_library(thirdparty::glm ALIAS glm)
set_target_properties(uninstall PROPERTIES FOLDER "ThirdParty")