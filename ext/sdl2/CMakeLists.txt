set(SDL_STATIC_ENABLED_BY_DEFAULT ON CACHE INTERNAL "")
set(SDL_SHARED_ENABLED_BY_DEFAULT OFF CACHE INTERNAL "")

FetchContent_Declare (
        SDL2
        URL https://www.libsdl.org/release/SDL2-2.0.14.zip
)

FetchContent_MakeAvailable(SDL2)
add_library(thirdparty::sdl2 ALIAS SDL2-static)
add_library(thirdparty::sdl2-main ALIAS SDL2main)

SET_TARGET_PROPERTIES(SDL2-static PROPERTIES FOLDER "ThirdParty")
SET_TARGET_PROPERTIES(SDL2main PROPERTIES FOLDER "ThirdParty")