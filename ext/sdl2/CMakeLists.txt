set(SDL_STATIC_ENABLED_BY_DEFAULT ON CACHE INTERNAL "")
set(SDL_SHARED_ENABLED_BY_DEFAULT OFF CACHE INTERNAL "")

fetchcontent_declare(
        SDL2
        URL https://www.libsdl.org/release/SDL2-2.0.14.zip
)

fetchcontent_makeavailable(SDL2)
add_library(thirdparty::sdl2 ALIAS SDL2-static)
add_library(thirdparty::sdl2-main ALIAS SDL2main)

set_target_properties(SDL2-static PROPERTIES FOLDER "ThirdParty")
set_target_properties(SDL2main PROPERTIES FOLDER "ThirdParty")