FetchContent_Declare(nlohmann_json_ext URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(nlohmann_json_ext)

add_library(thirdparty::json ALIAS nlohmann_json)
