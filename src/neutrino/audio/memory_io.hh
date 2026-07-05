/// @file memory_io.hh
/// @brief In-memory musac io streams over shared byte buffers.
///
/// std::istream sources are slurped into a buffer at load time: musac
/// decodes lazily (so a live istream would have to outlive playback), needs
/// seekable input, and concurrent sfx channels each need their own view of
/// the data. A shared buffer gives all three.

#pragma once

#include <cstdint>
#include <istream>
#include <memory>
#include <vector>

#include <musac/sdk/io_stream.hh>

namespace neutrino::audio_detail {

// Read an istream to EOF. Never nullptr; empty vector on a failed stream.
std::shared_ptr<const std::vector<uint8_t>> read_all(std::istream& is);

// Wrap a shared buffer into a musac io_stream that keeps the buffer alive.
std::unique_ptr<musac::io_stream> io_from_buffer(std::shared_ptr<const std::vector<uint8_t>> data);

} // namespace neutrino::audio_detail
