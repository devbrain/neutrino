//
// Created by igor on 17/06/2021.
//

#include <hal/sdl/sdl2.hh>

#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#elif defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4267 4244)
#endif
#define STBI_NO_STDIO
#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"


