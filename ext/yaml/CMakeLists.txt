fetchcontent_declare(
        RYAML
        GIT_REPOSITORY "https://github.com/biojppm/rapidyaml.git"
        GIT_TAG master     # or a particular branch, tag or commit
        GIT_SHALLOW FALSE  # ensure submodules are checked out
)
fetchcontent_makeavailable(RYAML)

set_target_properties(ryml PROPERTIES FOLDER "ThirdParty")
add_library(thirdparty::ryml ALIAS ryml)