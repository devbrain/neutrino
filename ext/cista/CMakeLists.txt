fetchcontent_declare(
        FMT
        URL "https://github.com/fmtlib/fmt/releases/download/11.0.2/fmt-11.0.2.zip"
        DOWNLOAD_EXTRACT_TIMESTAMP FALSE
)
fetchcontent_makeavailable(FMT)

fetchcontent_declare(
        CISTA
        URL "https://github.com/felixguendling/cista/releases/download/v0.15/cista.h"
        DOWNLOAD_NO_EXTRACT TRUE

)
fetchcontent_makeavailable(CISTA)
FetchContent_GetProperties(CISTA SOURCE_DIR CISTA_SOURCE_DIR)

add_library(cista INTERFACE)
target_include_directories(cista  INTERFACE ${CISTA_SOURCE_DIR})
target_link_libraries(cista INTERFACE fmt::fmt)
add_library(thirdparty::cista ALIAS cista)
set_target_properties(cista PROPERTIES FOLDER "ThirdParty")
