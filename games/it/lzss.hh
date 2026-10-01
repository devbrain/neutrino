//
// Created by igor on 30/09/2026.
//

#pragma once

#include <vector>
#include <span>
#include <cstdint>
#include <cstddef>

/**
 * @brief Decompresses LZSS-compressed asset blocks from IT resource streams.
 *
 * Implements the decompression algorithm from sub_409604:
 * - 1-byte control flags per 8 tokens (processed LSB to MSB).
 * - Bit 1: Literal byte copied directly from the input stream.
 * - Bit 0: 2-byte back-reference word (12-bit offset, 4-bit length + 3).
 *
 * @param comp_data Compressed input byte sequence.
 * @param decomp_size Expected uncompressed size (if > 0, output is clamped/reserved).
 * @return Decompressed byte vector.
 */
std::vector<uint8_t> decompress_lzss(std::span<const uint8_t> comp_data, std::size_t decomp_size = 0);

inline std::vector<uint8_t> decompress_lzss(const uint8_t* data, std::size_t size, std::size_t decomp_size = 0) {
    return decompress_lzss(std::span<const uint8_t>(data, size), decomp_size);
}
