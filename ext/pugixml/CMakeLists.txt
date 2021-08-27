fetchcontent_declare(
        PUGIXML
        GIT_REPOSITORY "https://github.com/zeux/pugixml.git"
)
fetchcontent_makeavailable(PUGIXML)

set_target_properties(pugixml-static PROPERTIES FOLDER "ThirdParty")
add_library(thirdparty::pugixml ALIAS pugixml)

