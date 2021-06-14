FetchContent_Declare(
        PUGIXML
        GIT_REPOSITORY "https://github.com/zeux/pugixml.git"
)
FetchContent_MakeAvailable(PUGIXML)

SET_TARGET_PROPERTIES(pugixml-static PROPERTIES FOLDER "ThirdParty")
add_library(thirdparty::pugixml ALIAS pugixml)

