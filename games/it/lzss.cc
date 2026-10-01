//
// Created by igor on 30/09/2026.
//

#include "lzss.hh"

std::vector<uint8_t> decompress_lzss(std::span<const uint8_t> comp_data, std::size_t decomp_size) {
    std::vector<uint8_t> out;
    if (decomp_size > 0) {
        out.reserve(decomp_size);
    }

    std::size_t in_idx = 0;
    const std::size_t comp_size = comp_data.size();

    while (in_idx < comp_size) {
        uint8_t flags = comp_data[in_idx++];
        for (int bit = 0; bit < 8; ++bit) {
            if (in_idx >= comp_size) {
                break;
            }

            if (flags & (1 << bit)) {
                // Literal byte
                out.push_back(comp_data[in_idx++]);
            } else {
                // 2-byte LZSS reference:
                // v10 = (low_byte) | (high_byte << 8)
                // offset (12 bits) = (v10 & 0xFF) | ((v10 & 0xF000) >> 4)
                // length (4 bits)  = ((v10 & 0x0F00) >> 8) + 3
                if (in_idx + 1 >= comp_size) {
                    break;
                }
                const uint16_t v10 = static_cast<uint16_t>(comp_data[in_idx]) |
                                     (static_cast<uint16_t>(comp_data[in_idx + 1]) << 8);
                in_idx += 2;

                const std::size_t offset = (v10 & 0xFF) | ((v10 & 0xF000) >> 4);
                const std::size_t length = ((v10 & 0x0F00) >> 8) + 3;

                for (std::size_t j = 0; j < length; ++j) {
                    if (offset == 0 || offset > out.size()) {
                        out.push_back(0);
                    } else {
                        out.push_back(out[out.size() - offset]);
                    }
                }
            }

            if (decomp_size > 0 && out.size() >= decomp_size) {
                out.resize(decomp_size);
                return out;
            }
        }
    }

    if (decomp_size > 0 && out.size() > decomp_size) {
        out.resize(decomp_size);
    }
    return out;
}
