set(SDL_STATIC_ENABLED_BY_DEFAULT ON CACHE INTERNAL "")

fetchcontent_declare(
        RYAML
        GIT_REPOSITORY "https://github.com/biojppm/rapidyaml.git"
        GIT_TAG master     # or a particular branch, tag or commit
        GIT_SHALLOW FALSE  # ensure submodules are checked out
        UPDATE_COMMAND ""
)
fetchcontent_makeavailable(RYAML)

set_target_properties(ryml PROPERTIES FOLDER "ThirdParty")
#set_target_properties(ryml-unistall PROPERTIES FOLDER "ThirdParty")
add_library(thirdparty::ryml ALIAS ryml)
set_thirdparty(ryml)
