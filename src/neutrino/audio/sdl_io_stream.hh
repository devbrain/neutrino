//
// Created by igor on 02/07/2026.
//

#pragma once

#include <memory>
#include <string>
#include <musac/sdk/io_stream.hh>
#include <SDL3/SDL.h>

namespace neutrino::audio_detail {

// Wrap an existing SDL_IOStream into a musac io_stream
std::unique_ptr<musac::io_stream> io_from_sdl(SDL_IOStream* sdl_stream);

// Open a file via SDL3 and wrap it into a musac io_stream
std::unique_ptr<musac::io_stream> io_from_file(const std::string& path);

} // namespace neutrino::audio_detail
