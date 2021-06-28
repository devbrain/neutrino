set(BROTLI_BUNDLED_MODE ON CACHE INTERNAL "")
set(BROTLI_DISABLE_TESTS ON CACHE INTERNAL "")

FetchContent_Declare (
        brotli-ext
        GIT_REPOSITORY https://github.com/google/brotli.git
)

FetchContent_MakeAvailable(brotli-ext)
add_library(thirdparty::brotli-common ALIAS brotlicommon-static)
add_library(thirdparty::brotli-dec ALIAS brotlidec-static)
add_library(thirdparty::brotli-enc ALIAS brotlienc-static)


SET_TARGET_PROPERTIES(brotlicommon-static PROPERTIES FOLDER "ThirdParty")
SET_TARGET_PROPERTIES(brotlidec-static PROPERTIES FOLDER "ThirdParty")
SET_TARGET_PROPERTIES(brotlienc-static PROPERTIES FOLDER "ThirdParty")
