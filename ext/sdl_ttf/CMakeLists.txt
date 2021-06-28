set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")

FetchContent_Declare (
        SDL_TTF
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL_ttf.git"
        GIT_TAG main
)

FetchContent_MakeAvailable(SDL_TTF)
add_library(SDL2::SDL2-static ALIAS SDL2-static)
add_library(thirdparty::sdl-ttf ALIAS SDL2_ttf)


SET_TARGET_PROPERTIES(SDL2_ttf PROPERTIES FOLDER "ThirdParty")
