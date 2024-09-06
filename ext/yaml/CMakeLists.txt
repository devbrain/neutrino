fetchcontent_declare(
        YAML-CPP
        GIT_REPOSITORY "https://github.com/jbeder/yaml-cpp.git"
        GIT_TAG master     # or a particular branch, tag or commit
        GIT_SHALLOW FALSE  # ensure submodules are checked out
)
fetchcontent_makeavailable(YAML-CPP)

add_library(thirdparty::yml ALIAS yaml-cpp)
set_target_properties(yaml-cpp PROPERTIES FOLDER "ThirdParty")
