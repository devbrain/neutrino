FetchContent_Declare(
        GLM
        GIT_REPOSITORY "https://github.com/g-truc/glm.git"
)
FetchContent_MakeAvailable(GLM)
add_library(thirdparty::glm ALIAS glm)
SET_TARGET_PROPERTIES(uninstall PROPERTIES FOLDER "ThirdParty")