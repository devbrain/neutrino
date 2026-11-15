//
// Created by igor on 02/07/2026.
//

#pragma once

#include <memory>
#include <string>
#include <musac/sdk/io_stream.hh>
#include <sdlpp/io/iostream.hh>

namespace neutrino::audio_detail {

// Wrap an SDL stream into a musac io_stream (taking ownership); nullptr for an invalid stream
std::unique_ptr<musac::io_stream> io_from_sdl(sdlpp::iostream stream);

// Open a file via SDL and wrap it into a musac io_stream; nullptr if it cannot be opened
std::unique_ptr<musac::io_stream> io_from_file(const std::string& path);

} // namespace neutrino::audio_detail
