fetchcontent_declare(json
        GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
        GIT_TAG v3.9.1)
fetchcontent_makeavailable(json)

add_library(thirdparty::json ALIAS nlohmann_json)
